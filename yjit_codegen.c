// This file is a fragment of the yjit.o compilation unit. See yjit.c.
#include "internal.h"
#include "gc.h"
#include "internal/compile.h"
#include "internal/class.h"
#include "internal/hash.h"
#include "internal/object.h"
#include "internal/sanitizers.h"
#include "internal/string.h"
#include "internal/struct.h"
#include "internal/variable.h"
#include "internal/re.h"
#include "probes.h"
#include "probes_helper.h"
#include "yjit.h"
#include "yjit_iface.h"
#include "yjit_core.h"
#include "yjit_codegen.h"
#include "yjit_asm.h"

// Map from YARV opcodes to code generation functions
static codegen_fn gen_fns[VM_INSTRUCTION_SIZE] = { NULL };

// Map from method entries to code generation functions
static st_table *yjit_method_codegen_table = NULL;

// Code for exiting back to the interpreter from the leave instruction
static void *leave_exit_code;

// Code for full logic of returning from C method and exiting to the interpreter
static uint32_t outline_full_cfunc_return_pos;

// For implementing global code invalidation
struct codepage_patch {
    uint32_t inline_patch_pos;
    uint32_t outlined_target_pos;
};

typedef rb_darray(struct codepage_patch) patch_array_t;

static patch_array_t global_inval_patches = NULL;

// Get the current instruction's opcode
static int
jit_get_opcode(jitstate_t *jit)
{
    return jit->opcode;
}

// Get the index of the next instruction
static uint32_t
jit_next_insn_idx(jitstate_t *jit)
{
    return jit->insn_idx + insn_len(jit_get_opcode(jit));
}

// Check if we are compiling the instruction at the stub PC
// Meaning we are compiling the instruction that is next to execute
static bool
jit_at_current_insn(jitstate_t *jit)
{
    const VALUE *ec_pc = jit->ec->cfp->pc;
    return (ec_pc == jit->pc);
}

// Record the current codeblock write position for rewriting into a jump into
// the outlined block later. Used to implement global code invalidation.
static void
record_global_inval_patch(const codeblock_t *cb, uint32_t outline_block_target_pos)
{
    struct codepage_patch patch_point = { cb->write_pos, outline_block_target_pos };
    if (!rb_darray_append(&global_inval_patches, patch_point)) rb_bug("allocation failed");
}

#if YJIT_STATS

// Add a comment at the current position in the code block
static void
_add_comment(codeblock_t *cb, const char *comment_str)
{
    // We can't add comments to the outlined code block
    if (cb == ocb)
        return;

    // Avoid adding duplicate comment strings (can happen due to deferred codegen)
    size_t num_comments = rb_darray_size(yjit_code_comments);
    if (num_comments > 0) {
        struct yjit_comment last_comment = rb_darray_get(yjit_code_comments, num_comments - 1);
        if (last_comment.offset == cb->write_pos && strcmp(last_comment.comment, comment_str) == 0) {
            return;
        }
    }

    struct yjit_comment new_comment = (struct yjit_comment){ cb->write_pos, comment_str };
    rb_darray_append(&yjit_code_comments, new_comment);
}

// Comments for generated machine code
#define ADD_COMMENT(cb, comment) _add_comment((cb), (comment))

// Verify the ctx's types and mappings against the compile-time stack, self,
// and locals.
static void
verify_ctx(jitstate_t *jit, ctx_t *ctx)
{
    // Only able to check types when at current insn
    RUBY_ASSERT(jit_at_current_insn(jit));

    VALUE self_val = jit_peek_at_self(jit, ctx);
    if (type_diff(yjit_type_of_value(self_val), ctx->self_type) == INT_MAX) {
        rb_bug("verify_ctx: ctx type (%s) incompatible with actual value of self: %s", yjit_type_name(ctx->self_type), rb_obj_info(self_val));
    }

    for (int i = 0; i < ctx->stack_size && i < MAX_TEMP_TYPES; i++) {
        temp_type_mapping_t learned = ctx_get_opnd_mapping(ctx, OPND_STACK(i));
        VALUE val = jit_peek_at_stack(jit, ctx, i);
        val_type_t detected = yjit_type_of_value(val);

        if (learned.mapping.kind == TEMP_SELF) {
            if (self_val != val) {
                rb_bug("verify_ctx: stack value was mapped to self, but values did not match\n"
                        "  stack: %s\n"
                        "  self: %s",
                        rb_obj_info(val),
                        rb_obj_info(self_val));
            }
        }

        if (learned.mapping.kind == TEMP_LOCAL) {
            int local_idx = learned.mapping.idx;
            VALUE local_val = jit_peek_at_local(jit, ctx, local_idx);
            if (local_val != val) {
                rb_bug("verify_ctx: stack value was mapped to local, but values did not match\n"
                        "  stack: %s\n"
                        "  local %i: %s",
                        rb_obj_info(val),
                        local_idx,
                        rb_obj_info(local_val));
            }
        }

        if (type_diff(detected, learned.type) == INT_MAX) {
            rb_bug("verify_ctx: ctx type (%s) incompatible with actual value on stack: %s", yjit_type_name(learned.type), rb_obj_info(val));
        }
    }

    int32_t local_table_size = jit->iseq->body->local_table_size;
    for (int i = 0; i < local_table_size && i < MAX_TEMP_TYPES; i++) {
        val_type_t learned = ctx->local_types[i];
        VALUE val = jit_peek_at_local(jit, ctx, i);
        val_type_t detected = yjit_type_of_value(val);

        if (type_diff(detected, learned) == INT_MAX) {
            rb_bug("verify_ctx: ctx type (%s) incompatible with actual value of local: %s", yjit_type_name(learned), rb_obj_info(val));
        }
    }
}

#else

#define ADD_COMMENT(cb, comment) ((void)0)
#define verify_ctx(jit, ctx) ((void)0)

#endif // if YJIT_STATS

#if YJIT_STATS

// Increment a profiling counter with counter_name
#define GEN_COUNTER_INC(cb, counter_name) _gen_counter_inc(cb, &(yjit_runtime_counters . counter_name))
static void
_gen_counter_inc(codeblock_t *cb, int64_t *counter)
{
    if (!rb_yjit_opts.gen_stats) return;

    // Use REG1 because there might be return value in REG0
    mov(cb, REG1, const_ptr_opnd(counter));
    cb_write_lock_prefix(cb); // for ractors.
    add(cb, mem_opnd(64, REG1, 0), imm_opnd(1));
}

// Increment a counter then take an existing side exit.
#define COUNTED_EXIT(jit, side_exit, counter_name) _counted_side_exit(jit, side_exit, &(yjit_runtime_counters . counter_name))
static uint8_t *
_counted_side_exit(jitstate_t* jit, uint8_t *existing_side_exit, int64_t *counter)
{
    if (!rb_yjit_opts.gen_stats) return existing_side_exit;

    uint8_t *start = cb_get_ptr(jit->ocb, jit->ocb->write_pos);
    _gen_counter_inc(jit->ocb, counter);
    jmp_ptr(jit->ocb, existing_side_exit);
    return start;
}

#else

#define GEN_COUNTER_INC(cb, counter_name) ((void)0)
#define COUNTED_EXIT(jit, side_exit, counter_name) side_exit

#endif // if YJIT_STATS

static uint32_t yjit_gen_exit(VALUE *exit_pc, ctx_t *ctx, codeblock_t *cb);

// Generate a stubbed unconditional jump to the next bytecode instruction.
// Blocks that are part of a guard chain can use this to share the same successor.
static void
jit_jump_to_next_insn(jitstate_t *jit, const ctx_t *current_context)
{
    // Reset the depth since in current usages we only ever jump to to
    // chain_depth > 0 from the same instruction.
    ctx_t reset_depth = *current_context;
    reset_depth.chain_depth = 0;

    blockid_t jump_block = { jit->iseq, jit_next_insn_idx(jit) };

    // We are at the end of the current instruction. Record the boundary.
    if (jit->record_boundary_patch_point) {
        uint32_t exit_pos = yjit_gen_exit(jit->pc + insn_len(jit->opcode), &reset_depth, jit->ocb);
        record_global_inval_patch(jit->cb, exit_pos);
        jit->record_boundary_patch_point = false;
    }

    // Generate the jump instruction
    gen_direct_jump(
        jit,
        &reset_depth,
        jump_block
    );
}

// Compile a sequence of bytecode instructions for a given basic block version.
// Part of gen_block_version().
static block_t *
gen_single_block(blockid_t blockid, const ctx_t *start_ctx, rb_execution_context_t *ec)
{
    RUBY_ASSERT(cb != NULL);
    verify_blockid(blockid);

    // Allocate the new block
    block_t *block = calloc(1, sizeof(block_t));
    if (!block) {
        return NULL;
    }

    // Copy the starting context to avoid mutating it
    ctx_t ctx_copy = *start_ctx;
    ctx_t *ctx = &ctx_copy;

    // Limit the number of specialized versions for this block
    *ctx = limit_block_versions(blockid, ctx);

    // Save the starting context on the block.
    block->blockid = blockid;
    block->ctx = *ctx;

    RUBY_ASSERT(!(blockid.idx == 0 && start_ctx->stack_size > 0));

    const rb_iseq_t *iseq = block->blockid.iseq;
    const unsigned int iseq_size = iseq->body->iseq_size;
    uint32_t insn_idx = block->blockid.idx;
    const uint32_t starting_insn_idx = insn_idx;

    // Initialize a JIT state object
    jitstate_t jit = {
        .cb = cb,
        .ocb = ocb,
        .block = block,
        .iseq = iseq,
        .ec = ec
    };

    // Mark the start position of the block
    block->start_addr = cb_get_write_ptr(cb);

    // For each instruction to compile
    while (insn_idx < iseq_size) {
        // Get the current pc and opcode
        VALUE *pc = yjit_iseq_pc_at_idx(iseq, insn_idx);
        int opcode = yjit_opcode_at_pc(iseq, pc);
        RUBY_ASSERT(opcode >= 0 && opcode < VM_INSTRUCTION_SIZE);

        // opt_getinlinecache wants to be in a block all on its own. Cut the block short
        // if we run into it. See gen_opt_getinlinecache() for details.
        if (opcode == BIN(opt_getinlinecache) && insn_idx > starting_insn_idx) {
            jit_jump_to_next_insn(&jit, ctx);
            break;
        }

        // Set the current instruction
        jit.insn_idx = insn_idx;
        jit.opcode = opcode;
        jit.pc = pc;
        jit.side_exit_for_pc = NULL;

        // If previous instruction requested to record the boundary
        if (jit.record_boundary_patch_point) {
            // Generate an exit to this instruction and record it
            uint32_t exit_pos = yjit_gen_exit(jit.pc, ctx, ocb);
            record_global_inval_patch(cb, exit_pos);
            jit.record_boundary_patch_point = false;
        }

        // Verify our existing assumption (DEBUG)
        if (jit_at_current_insn(&jit)) {
            verify_ctx(&jit, ctx);
        }

        // Lookup the codegen function for this instruction
        codegen_fn gen_fn = gen_fns[opcode];
        codegen_status_t status = YJIT_CANT_COMPILE;
        if (gen_fn) {
            if (0) {
                fprintf(stderr, "compiling %d: %s\n", insn_idx, insn_name(opcode));
                print_str(cb, insn_name(opcode));
            }

            // :count-placement:
            // Count bytecode instructions that execute in generated code.
            // Note that the increment happens even when the output takes side exit.
            GEN_COUNTER_INC(cb, exec_instruction);

            // Add a comment for the name of the YARV instruction
            ADD_COMMENT(cb, insn_name(opcode));

            // Call the code generation function
            status = gen_fn(&jit, ctx, cb);
        }

        // If we can't compile this instruction
        // exit to the interpreter and stop compiling
        if (status == YJIT_CANT_COMPILE) {
            // TODO: if the codegen function makes changes to ctx and then return YJIT_CANT_COMPILE,
            // the exit this generates would be wrong. We could save a copy of the entry context
            // and assert that ctx is the same here.
            uint32_t exit_off = yjit_gen_exit(jit.pc, ctx, cb);

            // If this is the first instruction in the block, then we can use
            // the exit for block->entry_exit.
            if (insn_idx == block->blockid.idx) {
                block->entry_exit = cb_get_ptr(cb, exit_off);
            }
            break;
        }

        // For now, reset the chain depth after each instruction as only the
        // first instruction in the block can concern itself with the depth.
        ctx->chain_depth = 0;

        // Move to the next instruction to compile
        insn_idx += insn_len(opcode);

        // If the instruction terminates this block
        if (status == YJIT_END_BLOCK) {
            break;
        }
    }

    // Mark the end position of the block
    block->end_addr = cb_get_write_ptr(cb);

    // Store the index of the last instruction in the block
    block->end_idx = insn_idx;

    // We currently can't handle cases where the request is for a block that
    // doesn't go to the next instruction.
    RUBY_ASSERT(!jit.record_boundary_patch_point);

    // If code for the block doesn't fit, free the block and fail.
    if (cb->dropped_bytes || ocb->dropped_bytes) {
        yjit_free_block(block);
        return NULL;
    }

    if (YJIT_DUMP_MODE >= 2) {
        // Dump list of compiled instrutions
        fprintf(stderr, "Compiled the following for iseq=%p:\n", (void *)iseq);
        for (uint32_t idx = block->blockid.idx; idx < insn_idx; ) {
            int opcode = yjit_opcode_at_pc(iseq, yjit_iseq_pc_at_idx(iseq, idx));
            fprintf(stderr, "  %04d %s\n", idx, insn_name(opcode));
            idx += insn_len(opcode);
        }
    }

    return block;
}

static int tracing_invalidate_all_i(void *vstart, void *vend, size_t stride, void *data);
static void invalidate_all_blocks_for_tracing(const rb_iseq_t *iseq);

// Invalidate all generated code and patch C method return code to contain
// logic for firing the c_return TracePoint event. Once rb_vm_barrier()
// returns, all other ractors are pausing inside RB_VM_LOCK_ENTER(), which
// means they are inside a C routine. If there are any generated code on-stack,
// they are waiting for a return from a C routine. For every routine call, we
// patch in an exit after the body of the containing VM instruction. This makes
// it so all the invalidated code exit as soon as execution logically reaches
// the next VM instruction. The interpreter takes care of firing the tracing
// event if it so happens that the next VM instruction has one attached.
//
// The c_return event needs special handling as our codegen never outputs code
// that contains tracing logic. If we let the normal output code run until the
// start of the next VM instruction by relying on the patching scheme above, we
// would fail to fire the c_return event. The interpreter doesn't fire the
// event at an instruction boundary, so simply exiting to the interpreter isn't
// enough. To handle it, we patch in the full logic at the return address. See
// full_cfunc_return().
//
// In addition to patching, we prevent future entries into invalidated code by
// removing all live blocks from their iseq.
void
rb_yjit_tracing_invalidate_all(void)
{
    if (!rb_yjit_enabled_p()) return;

    // Stop other ractors since we are going to patch machine code.
    RB_VM_LOCK_ENTER();
    rb_vm_barrier();

    // Make it so all live block versions are no longer valid branch targets
    rb_objspace_each_objects(tracing_invalidate_all_i, NULL);

    // Apply patches
    const uint32_t old_pos = cb->write_pos;
    rb_darray_for(global_inval_patches, patch_idx) {
        struct codepage_patch patch = rb_darray_get(global_inval_patches, patch_idx);
        cb_set_pos(cb, patch.inline_patch_pos);
        uint8_t *jump_target = cb_get_ptr(ocb, patch.outlined_target_pos);
        gen_jump_ptr(cb, jump_target);
    }
    cb_set_pos(cb, old_pos);

    // Freeze invalidated part of the codepage. We only want to wait for
    // running instances of the code to exit from now on, so we shouldn't
    // change the code. There could be other ractors sleeping in
    // branch_stub_hit(), for example. We could harden this by changing memory
    // protection on the frozen range.
    RUBY_ASSERT_ALWAYS(yjit_codepage_frozen_bytes <= old_pos && "frozen bytes should increase monotonically");
    yjit_codepage_frozen_bytes = old_pos;

    cb_mark_all_executable(ocb);
    cb_mark_all_executable(cb);
    RB_VM_LOCK_LEAVE();
}

static int
tracing_invalidate_all_i(void *vstart, void *vend, size_t stride, void *data)
{
    VALUE v = (VALUE)vstart;
    for (; v != (VALUE)vend; v += stride) {
        void *ptr = asan_poisoned_object_p(v);
        asan_unpoison_object(v, false);

        if (rb_obj_is_iseq(v)) {
            rb_iseq_t *iseq = (rb_iseq_t *)v;
            invalidate_all_blocks_for_tracing(iseq);
        }

        asan_poison_object_if(ptr, v);
    }
    return 0;
}

static void
invalidate_all_blocks_for_tracing(const rb_iseq_t *iseq)
{
    struct rb_iseq_constant_body *body = iseq->body;
    if (!body) return; // iseq yet to be initialized

    ASSERT_vm_locking();

    // Empty all blocks on the iseq so we don't compile new blocks that jump to the
    // invalidted region.
    // TODO Leaking the blocks for now since we might have situations where
    // a different ractor is waiting in branch_stub_hit(). If we free the block
    // that ractor can wake up with a dangling block.
    rb_darray_for(body->yjit_blocks, version_array_idx) {
        rb_yjit_block_array_t version_array = rb_darray_get(body->yjit_blocks, version_array_idx);
        rb_darray_for(version_array, version_idx) {
            // Stop listening for invalidation events like basic operation redefinition.
            block_t *block = rb_darray_get(version_array, version_idx);
            yjit_unlink_method_lookup_dependency(block);
            yjit_block_assumptions_free(block);
        }
        rb_darray_free(version_array);
    }
    rb_darray_free(body->yjit_blocks);
    body->yjit_blocks = NULL;

#if USE_MJIT
    // Reset output code entry point
    body->jit_func = NULL;
#endif
}

static void
yjit_reg_op(int opcode, codegen_fn gen_fn)
{
    RUBY_ASSERT(opcode >= 0 && opcode < VM_INSTRUCTION_SIZE);
    // Check that the op wasn't previously registered
    RUBY_ASSERT(gen_fns[opcode] == NULL);

    gen_fns[opcode] = gen_fn;
}

static void reg_supported_opcodes();
static void reg_supported_methods();
static uint8_t *yjit_gen_leave_exit(codeblock_t *cb);
static void gen_full_cfunc_return(void);

void
yjit_init_codegen(void)
{
    // Initialize the code blocks
    uint32_t mem_size = rb_yjit_opts.exec_mem_size * 1024 * 1024;
    uint8_t *mem_block = alloc_exec_mem(mem_size);

    cb = &block;
    cb_init(cb, mem_block, mem_size/2);

    ocb = &outline_block;
    cb_init(ocb, mem_block + mem_size/2, mem_size/2);

    // Generate the interpreter exit code for leave
    leave_exit_code = yjit_gen_leave_exit(cb);

    // Generate full exit code for C func
    gen_full_cfunc_return();
    cb_mark_all_executable(cb);

    reg_supported_opcodes();
    yjit_method_codegen_table = st_init_numtable();
    reg_supported_methods();
}

#if YJIT_TARGET_ARCH == YJIT_ARCH_X86_64
# include "yjit_codegen_x86_64.c"
#elif YJIT_TARGET_ARCH == YJIT_ARCH_ARM64
# include "yjit_codegen_arm64.c"
#endif