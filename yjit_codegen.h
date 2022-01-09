#ifndef YJIT_CODEGEN_H
#define YJIT_CODEGEN_H 1

typedef enum codegen_status {
    YJIT_END_BLOCK,
    YJIT_KEEP_COMPILING,
    YJIT_CANT_COMPILE
} codegen_status_t;

// Code generation function signature
typedef codegen_status_t (*codegen_fn)(jitstate_t *jit, ctx_t *ctx, codeblock_t *cb);

static void jit_ensure_block_entry_exit(jitstate_t *jit);

static uint8_t *yjit_entry_prologue(codeblock_t *cb, const rb_iseq_t *iseq);

static block_t *gen_single_block(blockid_t blockid, const ctx_t *start_ctx, rb_execution_context_t *ec);

static void gen_code_for_exit_from_stub(void);

static void gen_call_branch_stub_hit(branch_t *branch, uint32_t target_idx);

static void gen_jump_branch(codeblock_t *cb, uint8_t *target0, uint8_t *target1, uint8_t shape);
static inline void gen_jump_ptr(codeblock_t *cb, uint8_t *target);

static void yjit_init_codegen(void);

#endif // #ifndef YJIT_CODEGEN_H
