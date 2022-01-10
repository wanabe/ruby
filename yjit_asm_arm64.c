// This file is a fragment of the yjit.o compilation unit. See yjit.c.
//
// Note that the definition for some of these functions don't specify
// static inline, but their declaration in yjit_asm.h do. The resulting
// linkage is the same as if they both specify. The relevant sections in
// N1256 is 6.2.2p4, 6.2.2p5, and 6.7.4p5.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>

// For mmapp(), sysconf()
#ifndef _WIN32
#include <unistd.h>
#include <sys/mman.h>
#endif

#include "yjit_arch.h"
#include "yjit_asm.h"

arm64opnd_t mem_opnd(uint32_t num_bits, arm64opnd_t base_reg, int32_t disp)
{
    arm64opnd_t opnd = {
        OPND_MEM,
        num_bits,
        .as.mem = { base_reg.as.reg.reg_no, ADDR_OFFSET, disp}
    };

    return opnd;
}

arm64opnd_t mem_pre_opnd(uint32_t num_bits, arm64opnd_t base_reg, int32_t disp)
{
    arm64opnd_t opnd = {
        OPND_MEM,
        num_bits,
        .as.mem = { base_reg.as.reg.reg_no, ADDR_PRE_IDX, disp}
    };

    return opnd;
}

arm64opnd_t mem_post_opnd(uint32_t num_bits, arm64opnd_t base_reg, int32_t disp)
{
    arm64opnd_t opnd = {
        OPND_MEM,
        num_bits,
        .as.mem = { base_reg.as.reg.reg_no, ADDR_POST_IDX, disp}
    };

    return opnd;
}

arm64opnd_t imm_opnd(int64_t imm)
{
    arm64opnd_t opnd = {
        OPND_IMM,
        sig_imm_size(imm),
        .as.imm = imm
    };

    return opnd;
}

arm64opnd_t imm_shift_opnd(uint32_t imm, enum ShiftType shift_type, uint8_t shift_num)
{
    arm64opnd_t opnd = {
        OPND_IMM_SHIFT,
        unsig_imm_size(imm),
        .as.imm_shift = { imm, shift_type, shift_num}
    };

    return opnd;
}

/// ldr - Data load operation
/// bit     7 6 5 4 3 2 1 0
void ldr(codeblock_t *cb, arm64opnd_t dst, arm64opnd_t src)
{
    assert (src.type == OPND_MEM);
    assert (dst.type == OPND_REG);
    assert (dst.num_bits == 64);

    if (src.as.mem.disp % 8 != 0) {
        rb_bug("unimplemented: ldr with not 8 scaled offset");
    }
    if (src.as.mem.disp < 0) {
        rb_bug("unimplemented: ldr with negative offset");
    }
    if (src.as.mem.addressing != ADDR_OFFSET) {
        rb_bug("unimplemented: ldr with not-offset addressing");
    }

    uint16_t uimm12 = src.as.mem.disp >> 3;
    // 31-24:  1 1 1 1 1 0 0 1
    // 23-16:  0 1 u_i_m_m_m_m
    // 15-08: _m_m_m_m_1_2 R_n
    // 07-00: _n_n_n R_t_t_t_t
    cb_write_bytes(cb, 4,
      dst.as.reg.reg_no | (src.as.mem.base_reg_no & 0x7) << 5,
      src.as.mem.base_reg_no >> 3 | (uimm12 & 0x3f) << 2,
      uimm12 >> 6 | 0x40,
      0xf9
    );
}

/// str - Data store operation
void str(codeblock_t *cb, arm64opnd_t src, arm64opnd_t dst)
{
    assert (src.type == OPND_REG);
    assert (dst.type == OPND_MEM);
    assert (src.num_bits == 64);

    if (dst.as.mem.disp % 8 != 0) {
        rb_bug("unimplemented: str with not 8 scaled offset");
    }
    if (dst.as.mem.disp < 0) {
        rb_bug("unimplemented: str with negative offset");
    }
    if (dst.as.mem.addressing != ADDR_OFFSET) {
        rb_bug("unimplemented: ldr with not-offset addressing");
    }

    uint16_t uimm12 = dst.as.mem.disp >> 3;
    // 31-24:  1 1 1 1 1 0 0 1
    // 23-16:  0 0 u_i_m_m_m_m
    // 15-08: _m_m_m_m_1_2 R_n
    // 07-00: _n_n_n R_t_t_t_t
    cb_write_bytes(cb, 4,
      src.as.reg.reg_no | (dst.as.mem.base_reg_no & 0x7) << 5,
      dst.as.mem.base_reg_no >> 3 | (uimm12 & 0x3f) << 2,
      uimm12 >> 6,
      0xf9
    );
}

/// ldp - Data pair load operation
void ldp(codeblock_t *cb, arm64opnd_t dst1, arm64opnd_t dst2, arm64opnd_t src)
{
    assert (dst1.num_bits == dst2.num_bits);
    assert (dst1.type == OPND_REG);
    assert (dst2.type == OPND_REG);
    assert (src.type == OPND_MEM);

    bool z = dst1.num_bits == 64;
    uint8_t imm7 = (src.as.mem.disp / (z ? 8 : 4)) & 0x7f;

    /// bit     7 6 5 4 3 2 1 0
    /// 31-24:  z 0 1 0 1 0 A_d
    /// 23-16: _r 1 i_m_m_7_7_7
    /// 15-08: _7 R_t_2_2_2 R_n
    /// 07-00: _n_n_n R_t_1_1_1
    cb_write_bytes(cb, 4,
      dst1.as.reg.reg_no | (src.as.mem.base_reg_no & 0x7) << 5,
      src.as.mem.base_reg_no >> 3 | dst2.as.reg.reg_no << 2 | (imm7 & 0x1) << 7,
      imm7 >> 1 | 0x40 | (src.as.mem.addressing & 0x1) << 7,
      src.as.mem.addressing >> 1 | (z ? 0xa8 : 0x28)
    );
}

/// stp - Data pair store operation
void stp(codeblock_t *cb, arm64opnd_t src1, arm64opnd_t src2, arm64opnd_t dst)
{
    assert (src1.num_bits == src2.num_bits);
    assert (src1.type == OPND_REG);
    assert (src2.type == OPND_REG);
    assert (dst.type == OPND_MEM);

    bool z = src1.num_bits == 64;
    uint8_t imm7 = (dst.as.mem.disp / (z ? 8 : 4)) & 0x7f;

    /// bit     7 6 5 4 3 2 1 0
    /// 31-24:  z 0 1 0 1 0 A_d
    /// 23-16: _r 0 i_m_m_7_7_7
    /// 15-08: _7 R_t_2_2_2 R_n
    /// 07-00: _n_n_n R_t_1_1_1
    cb_write_bytes(cb, 4,
      src1.as.reg.reg_no | (dst.as.mem.base_reg_no & 0x7) << 5,
      dst.as.mem.base_reg_no >> 3 | src2.as.reg.reg_no << 2 | (imm7 & 0x1) << 7,
      imm7 >> 1 | (dst.as.mem.addressing & 0x1) << 7,
      dst.as.mem.addressing >> 1 | (z ? 0xa8 : 0x28)
    );
}

/// mov - Data move operation
void mov(codeblock_t *cb, arm64opnd_t dst, arm64opnd_t src)
{
    assert (dst.num_bits == src.num_bits);
    assert (dst.type == OPND_REG);
    assert (src.type == OPND_REG);

    if (src.as.reg.reg_type != REG_GP) {
        rb_bug("unimplemented: mov NOGP-GP");
    }

    bool z = dst.num_bits == 64;

    /// GP-GP   7 6 5 4 3 2 1 0
    /// 31-24:  z 0 1 0 1 0 1 0
    /// 23-16:  0 0 0 R_m_m_m_m
    /// 15-08:  0 0_0 0 0 0 1 1
    /// 07-00:  1 1 1 R_d_d_d_d
    cb_write_bytes(cb, 4,
      dst.as.reg.reg_no | 0xe0,
      3,
      src.as.reg.reg_no,
      z ? 0xaa : 0x2a
    );
}

/// movz - Data move with zero operation
void movz(codeblock_t *cb, arm64opnd_t dst, arm64opnd_t src)
{
    assert (dst.type == OPND_REG);
    assert (src.type == OPND_IMM || src.type == OPND_IMM_SHIFT);

    bool z = dst.num_bits == 64;
    uint16_t imm16;
    uint8_t lsl;

    switch (src.type)
    {
    case OPND_IMM:
        lsl = 0;
        imm16 = src.as.imm;
        break;
    
    case OPND_IMM_SHIFT:
        lsl = src.as.imm_shift.shift_num;
        assert (src.as.imm_shift.shift_type == SHIFT_LSL);
        assert (lsl == 0 || lsl == 16 || lsl == 32 || lsl == 48);
        imm16 = src.as.imm_shift.imm;
        break;
    }

    ///         7 6 5 4 3 2 1 0
    /// 31-24:  z 1 0 1 0 0 1 0
    /// 23-16:  1 h_w i_m_m_m_m
    /// 15-08: _m_m_m_m_m_m_m_m
    /// 07-00: _m_1_6 R_d_d_d_d
    cb_write_bytes(cb, 4,
      dst.as.reg.reg_no | (src.as.unsig_imm & 0x0007) << 5,
      (imm16 & 0x07f8) >> 3,
      imm16 >> 11 | lsl << 1 | 0x80,
      z ? 0xd2 : 0x52
    );
}

/// movk - Data move with keep operation
void movk(codeblock_t *cb, arm64opnd_t dst, arm64opnd_t src)
{
    assert (dst.type == OPND_REG);
    assert (src.type == OPND_IMM || src.type == OPND_IMM_SHIFT);

    bool z = dst.num_bits == 64;
    uint16_t imm16;
    uint8_t lsl;

    switch (src.type)
    {
    case OPND_IMM:
        lsl = 0;
        imm16 = src.as.imm;
        break;
    
    case OPND_IMM_SHIFT:
        lsl = src.as.imm_shift.shift_num;
        assert (src.as.imm_shift.shift_type == SHIFT_LSL);
        assert (lsl == 0 || lsl == 16 || lsl == 32 || lsl == 48);
        imm16 = src.as.imm_shift.imm;
        break;
    }

    ///         7 6 5 4 3 2 1 0
    /// 31-24:  z 1 1 1 0 0 1 0
    /// 23-16:  1 h_w i_m_m_m_m
    /// 15-08: _m_m_m_m_m_m_m_m
    /// 07-00: _m_1_6 R_d_d_d_d
    cb_write_bytes(cb, 4,
      dst.as.reg.reg_no | (src.as.unsig_imm & 0x0007) << 5,
      (imm16 & 0x07f8) >> 3,
      imm16 >> 11 | lsl << 1 | 0x80,
      z ? 0xf2 : 0x72
    );
}

/// ret - Return operation
void ret(codeblock_t *cb, arm64opnd_t reg)
{
    assert (reg.type == OPND_REG);

    /// 31-24:  1 1 0 1 0 1 1 0
    /// 23-16:  0 1_0 1 1 1 1 1
    /// 15-08: _0 0 0 0 0 0 R_n
    /// 07-00: _n_n_n 0 0 0 0 0
    cb_write_bytes(cb, 4,
      (reg.as.reg.reg_no & 0x7) << 5,
      (reg.as.reg.reg_no & 0x18) >> 3,
      0x5f,
      0xd6
    );
}

/// add - Add operation
void add(codeblock_t *cb, arm64opnd_t dst, arm64opnd_t src1, arm64opnd_t src2)
{
    assert (dst.type == OPND_REG);
    assert (src1.type == OPND_REG);
    assert (src2.type == OPND_IMM || src2.type == OPND_IMM_SHIFT);

    bool z = dst.num_bits == 64;
    uint16_t imm12;
    uint8_t sh;

    switch (src2.type)
    {
    case OPND_IMM:
        sh = 0;
        imm12 = src2.as.imm & 0xfff;
        break;
    
    case OPND_IMM_SHIFT:
        sh = src2.as.imm_shift.shift_num / 12;
        assert (src2.as.imm_shift.shift_num == 0 || src2.as.imm_shift.shift_num == 12);
        assert (src2.as.imm_shift.shift_type == SHIFT_LSL);
        imm12 = src2.as.imm_shift.imm & 0xfff;
        break;
    }

    ///         7 6 5 4 3 2 1 0
    /// 31-24:  z 0 0 1 0 0 0 1
    /// 23-16:  s_h i_m_m_m_m_m
    /// 15-08: _m_m_m_m_1_2 R_n
    /// 07-00: _n_n_n R_d_d_d_d
    cb_write_bytes(cb, 4,
      dst.as.reg.reg_no | (src1.as.reg.reg_no & 0x7) << 5,
      src1.as.reg.reg_no >> 3 | (imm12 & 0x3f) << 2,
      imm12 >> 6 | sh,
      z ? 0x91 : 0x11
    );
}

void mov_uint64(codeblock_t *cb, arm64opnd_t dst, uint64_t imm)
{
    movz(cb, dst, imm_opnd(imm & 0xffff));
    imm = imm >> 16;
    if (imm == 0) return;
    movk(cb, dst, imm_shift_opnd(imm & 0xffff, SHIFT_LSL, 16));
    imm = imm >> 16;
    if (imm == 0) return;
    movk(cb, dst, imm_shift_opnd(imm & 0xffff, SHIFT_LSL, 32));
    imm = imm >> 16;
    if (imm == 0) return;
    movk(cb, dst, imm_shift_opnd(imm & 0xffff, SHIFT_LSL, 48));
}
