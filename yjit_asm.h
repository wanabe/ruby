#ifndef YJIT_ASM_H
#define YJIT_ASM_H 1

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Maximum number of labels to link
#define MAX_LABELS 32

// Maximum number of label references
#define MAX_LABEL_REFS 32

// Reference to an ASM label
typedef struct LabelRef
{
    // Position in the code block where the label reference exists
    uint32_t pos;

    // Label which this refers to
    uint32_t label_idx;

} labelref_t;

// Block of executable memory into which instructions can be written
typedef struct CodeBlock
{
    // Memory block
    // Users are advised to not use this directly.
    uint8_t *mem_block_;

    // Memory block size
    uint32_t mem_size;

    // Current writing position
    uint32_t write_pos;

    // Table of registered label addresses
    uint32_t label_addrs[MAX_LABELS];

    // Table of registered label names
    // Note that these should be constant strings only
    const char *label_names[MAX_LABELS];

    // References to labels
    labelref_t label_refs[MAX_LABEL_REFS];

    // Number of labels registeered
    uint32_t num_labels;

    // Number of references to labels
    uint32_t num_refs;


    // Keep track of the current aligned write position.
    // Used for changing protection when writing to the JIT buffer
    uint32_t current_aligned_write_pos;

    // Set if the assembler is unable to output some instructions,
    // for example, when there is not enough space or when a jump
    // target is too far away.
    bool dropped_bytes;

    // Flag to enable or disable comments
    bool has_asm;


} codeblock_t;

// 1 is not aligned so this won't match any pages
#define ALIGNED_WRITE_POSITION_NONE 1

// Code block functions
static inline void cb_init(codeblock_t *cb, uint8_t *mem_block, uint32_t mem_size);
static inline void cb_align_pos(codeblock_t *cb, uint32_t multiple);
static inline void cb_set_pos(codeblock_t *cb, uint32_t pos);
static inline void cb_set_write_ptr(codeblock_t *cb, uint8_t *code_ptr);
static inline uint8_t *cb_get_ptr(const codeblock_t *cb, uint32_t index);
static inline uint8_t *cb_get_write_ptr(const codeblock_t *cb);
static inline void cb_write_byte(codeblock_t *cb, uint8_t byte);
static inline void cb_write_bytes(codeblock_t *cb, uint32_t num_bytes, ...);
static inline void cb_write_int(codeblock_t *cb, uint64_t val, uint32_t num_bits);
static inline uint32_t cb_new_label(codeblock_t *cb, const char *name);
static inline void cb_write_label(codeblock_t *cb, uint32_t label_idx);
static inline void cb_label_ref(codeblock_t *cb, uint32_t label_idx);
static inline void cb_link_labels(codeblock_t *cb);
static inline void cb_mark_all_writeable(codeblock_t *cb);
static inline void cb_mark_position_writeable(codeblock_t *cb, uint32_t write_pos);
static inline void cb_mark_all_executable(codeblock_t *cb);

#if YJIT_TARGET_ARCH == YJIT_ARCH_X86_64
# include "yjit_asm_x86_64.h"
#elif YJIT_TARGET_ARCH == YJIT_ARCH_ARM64
# include "yjit_asm_arm64.h"
#endif

#endif
