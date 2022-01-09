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
    // TODO
    return 0;
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

    // TODO

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

    // TODO
    return NULL;
}


static void
reg_supported_opcodes() {
}

static void
reg_supported_methods() {
}