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

static void
gen_call_branch_stub_hit(branch_t *branch, uint32_t target_idx) {
    // TODO
    rb_bug("unimplemented: gen_call_branch_stub_hit");
}

static void
gen_jump_branch(codeblock_t *cb, uint8_t *target0, uint8_t *target1, uint8_t shape)
{
    // TODO
    rb_bug("unimplemented: gen_jump_branch");
}

static inline void
gen_jump_ptr(codeblock_t *cb, uint8_t *target)
{
    // TODO
    rb_bug("unimplemented: gen_jump_ptr");
}

// Generate an exit to return to the interpreter
static uint32_t
yjit_gen_exit(VALUE *exit_pc, ctx_t *ctx, codeblock_t *cb)
{
    const uint32_t code_pos = cb->write_pos;

    ADD_COMMENT(cb, "exit to interpreter");

    // Generate the code to exit to the interpreters
    // Write the adjusted SP back into the CFP
    if (ctx->sp_offset != 0) {
        add(cb, REG_SP, REG_SP, imm_opnd(ctx->sp_offset * sizeof(VALUE)));
        str(cb, REG_SP, member_opnd(REG_CFP, rb_control_frame_t, sp));
    }

    // Update CFP->PC
    mov_uint64(cb, X0, (uint64_t)exit_pc);
    str(cb, X0, member_opnd(REG_CFP, rb_control_frame_t, pc));

    // Accumulate stats about interpreter exits
#if YJIT_STATS
    if (rb_yjit_opts.gen_stats) {
        mov(cb, RDI, const_ptr_opnd(exit_pc));
        call_ptr(cb, RSI, (void *)&yjit_count_side_exit_op);
    }
#endif

    ldp(cb, REG_EC, REG_SP, mem_post_opnd(64, SP, 16));
    ldp(cb, X30, REG_CFP, mem_post_opnd(64, SP, 16));

    mov_uint64(cb, X0, Qundef);
    ret(cb, X30);

    return code_pos;
}

// Generate a continuation for gen_leave() that exits to the interpreter at REG_CFP->pc.
static uint8_t *
yjit_gen_leave_exit(codeblock_t *cb)
{
    uint8_t *code_ptr = cb_get_ptr(cb, cb->write_pos);

    // Note, gen_leave() fully reconstructs interpreter state and leaves the
    // return value in X0 before coming here.

    // Every exit to the interpreter should be counted
    GEN_COUNTER_INC(cb, leave_interp_return);

    ldp(cb, REG_EC, REG_SP, mem_post_opnd(64, SP, 16));
    ldp(cb, X30, REG_CFP, mem_post_opnd(64, SP, 16));

    ret(cb, X30);

    return code_ptr;
}

// Fill code_for_exit_from_stub. This is used by branch_stub_hit() to exit
// to the interpreter when it cannot service a stub by generating new code.
// Before coming here, branch_stub_hit() takes care of fully reconstructing
// interpreter state.
static void
gen_code_for_exit_from_stub(void)
{
    codeblock_t *cb = ocb;
    code_for_exit_from_stub = cb_get_ptr(cb, cb->write_pos);

    GEN_COUNTER_INC(cb, exit_from_branch_stub);

    // TODO
}

// Landing code for when c_return tracing is enabled. See full_cfunc_return().
static void
gen_full_cfunc_return(void)
{
    codeblock_t *cb = ocb;
    outline_full_cfunc_return_pos = ocb->write_pos;

    // This chunk of code expect REG_EC to be filled properly and
    // X0 to contain the return value of the C method.

    // TODO
}

/*
Compile an interpreter entry block to be inserted into an iseq
Returns `NULL` if compilation fails.
*/
static uint8_t *
yjit_entry_prologue(codeblock_t *cb, const rb_iseq_t *iseq)
{
    RUBY_ASSERT(cb != NULL);

    enum { MAX_PROLOGUE_SIZE = 1024 };

    // Check if we have enough executable memory
    if (cb->write_pos + MAX_PROLOGUE_SIZE >= cb->mem_size) {
        return NULL;
    }

    const uint32_t old_write_pos = cb->write_pos;

    // Align the current write position to cache line boundaries
    cb_align_pos(cb, 64);

    uint8_t *code_ptr = cb_get_ptr(cb, cb->write_pos);
    ADD_COMMENT(cb, "yjit entry");

    stp(cb, X30, REG_CFP, mem_pre_opnd(64, SP, -16));
    stp(cb, REG_EC, REG_SP, mem_pre_opnd(64, SP, -16));

    // We are passed EC and CFP
    mov(cb, REG_EC, X0);
    mov(cb, REG_CFP, X1);

    // Load the current SP from the CFP into REG_SP
    ldr(cb, REG_SP, member_opnd(REG_CFP, rb_control_frame_t, sp));

    // Setup cfp->jit_return
    // TODO: this could use an IP relative LEA instead of an 8 byte immediate
    mov_uint64(cb, REG0, (uint64_t)leave_exit_code);
    str(cb, REG0, member_opnd(REG_CFP, rb_control_frame_t, jit_return));

    // Verify MAX_PROLOGUE_SIZE
    RUBY_ASSERT_ALWAYS(cb->write_pos - old_write_pos <= MAX_PROLOGUE_SIZE);

    return code_ptr;
}

// Push a constant value to the stack, including type information.
// The constant may be a heap object or a special constant.
static void
jit_putobject(jitstate_t *jit, ctx_t *ctx, VALUE arg)
{
    val_type_t val_type = yjit_type_of_value(arg);
    arm64opnd_t stack_top = ctx_stack_push(ctx, val_type);

    if (SPECIAL_CONST_P(arg)) {
        // Immediates will not move and do not need to be tracked for GC
        // Thanks to this we can mov directly to memory when possible.

        mov_uint64(cb, REG0, arg);
        str(cb, REG0, stack_top);
    }
    else {
        // TODO
        rb_bug("unimplemented: jit_putobject for not SPECIAL_CONST");
    }
}

static codegen_status_t
gen_putnil(jitstate_t *jit, ctx_t *ctx, codeblock_t *cb)
{
    jit_putobject(jit, ctx, Qnil);
    return YJIT_KEEP_COMPILING;
}

static void
reg_supported_opcodes() {
    yjit_reg_op(BIN(putnil), gen_putnil);
}

static void
reg_supported_methods() {
}