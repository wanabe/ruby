#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

enum OpndType
{
    OPND_NONE,
    OPND_REG,
    OPND_IMM,
    OPND_IMM_SHIFT,
    OPND_MEM
};

enum RegType
{
    REG_GP,
    REG_STACK,
    REG_ZERO
};

enum AddrType
{
    ADDR_POST_IDX = 1,
    ADDR_OFFSET = 2,
    ADDR_PRE_IDX = 3,
};

enum ShiftType
{
    SHIFT_LSL
};

typedef struct Arm64Reg
{
    // Register type
    uint8_t reg_type;

    // Register index number
    uint8_t reg_no;

} arm64reg_t;

typedef struct Arm64Mem
{
    /// Base register number
    uint8_t base_reg_no;

    uint8_t addressing;

    /// Constant displacement from the base, not scaled
    int32_t disp;

} arm64mem_t;

typedef struct Arm64ImmShift
{
    /// Immediate value
    int32_t imm;

    uint8_t shift_type;

    uint8_t shift_num;

} arm64imm_shift_t;

typedef struct Arm64Opnd
{
    // Operand type
    uint8_t type;

    // Size in bits
    uint16_t num_bits;

    union
    {
        // Register operand
        arm64reg_t reg;

        // Memory operand
        arm64mem_t mem;

        arm64imm_shift_t imm_shift;

        // Signed immediate value
        int64_t imm;

        // Unsigned immediate value
        uint64_t unsig_imm;
    } as;

} arm64opnd_t;

#define yjit_opnd_t arm64opnd_t

// 64-bit GP registers
static const arm64opnd_t X0  = { OPND_REG, 64, .as.reg = { REG_GP,  0 }};
static const arm64opnd_t X1  = { OPND_REG, 64, .as.reg = { REG_GP,  1 }};
static const arm64opnd_t X2  = { OPND_REG, 64, .as.reg = { REG_GP,  2 }};
static const arm64opnd_t X3  = { OPND_REG, 64, .as.reg = { REG_GP,  3 }};
static const arm64opnd_t X4  = { OPND_REG, 64, .as.reg = { REG_GP,  4 }};
static const arm64opnd_t X5  = { OPND_REG, 64, .as.reg = { REG_GP,  5 }};
static const arm64opnd_t X6  = { OPND_REG, 64, .as.reg = { REG_GP,  6 }};
static const arm64opnd_t X7  = { OPND_REG, 64, .as.reg = { REG_GP,  7 }};
static const arm64opnd_t X8  = { OPND_REG, 64, .as.reg = { REG_GP,  8 }};
static const arm64opnd_t X9  = { OPND_REG, 64, .as.reg = { REG_GP,  9 }};
static const arm64opnd_t X10 = { OPND_REG, 64, .as.reg = { REG_GP, 10 }};
static const arm64opnd_t X11 = { OPND_REG, 64, .as.reg = { REG_GP, 11 }};
static const arm64opnd_t X12 = { OPND_REG, 64, .as.reg = { REG_GP, 12 }};
static const arm64opnd_t X13 = { OPND_REG, 64, .as.reg = { REG_GP, 13 }};
static const arm64opnd_t X14 = { OPND_REG, 64, .as.reg = { REG_GP, 14 }};
static const arm64opnd_t X15 = { OPND_REG, 64, .as.reg = { REG_GP, 15 }};
static const arm64opnd_t X16 = { OPND_REG, 64, .as.reg = { REG_GP, 16 }};
static const arm64opnd_t X17 = { OPND_REG, 64, .as.reg = { REG_GP, 17 }};
static const arm64opnd_t X18 = { OPND_REG, 64, .as.reg = { REG_GP, 18 }};
static const arm64opnd_t X19 = { OPND_REG, 64, .as.reg = { REG_GP, 19 }};
static const arm64opnd_t X20 = { OPND_REG, 64, .as.reg = { REG_GP, 20 }};
static const arm64opnd_t X21 = { OPND_REG, 64, .as.reg = { REG_GP, 21 }};
static const arm64opnd_t X22 = { OPND_REG, 64, .as.reg = { REG_GP, 22 }};
static const arm64opnd_t X23 = { OPND_REG, 64, .as.reg = { REG_GP, 23 }};
static const arm64opnd_t X24 = { OPND_REG, 64, .as.reg = { REG_GP, 24 }};
static const arm64opnd_t X25 = { OPND_REG, 64, .as.reg = { REG_GP, 25 }};
static const arm64opnd_t X26 = { OPND_REG, 64, .as.reg = { REG_GP, 26 }};
static const arm64opnd_t X27 = { OPND_REG, 64, .as.reg = { REG_GP, 27 }};
static const arm64opnd_t X28 = { OPND_REG, 64, .as.reg = { REG_GP, 28 }};
static const arm64opnd_t X29 = { OPND_REG, 64, .as.reg = { REG_GP, 29 }};
static const arm64opnd_t X30 = { OPND_REG, 64, .as.reg = { REG_GP, 30 }};

static const arm64opnd_t SP  = { OPND_REG, 64, .as.reg = { REG_STACK, 31 }};
static const arm64opnd_t XZR = { OPND_REG, 64, .as.reg = { REG_ZERO,  31 }};

// Immediate number operand
static inline arm64opnd_t imm_opnd(int64_t val);
