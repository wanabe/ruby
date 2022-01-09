enum OpndType
{
    OPND_NONE,
    OPND_REG,
    OPND_IMM,
    OPND_MEM
};

enum RegType
{
    REG_GP
};

typedef struct Arm64Reg
{
    // Register type
    uint8_t reg_type;

    // Register index number
    uint8_t reg_no;

} arm64reg_t;

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

        // Signed immediate value
        int64_t imm;

        // Unsigned immediate value
        uint64_t unsig_imm;
    } as;

} arm64opnd_t;

#define yjit_opnd_t arm64opnd_t

// 64-bit GP registers
static const arm64opnd_t X19 = { OPND_REG, 64, .as.reg = { REG_GP, 19 }};
