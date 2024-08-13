#![allow(dead_code)] // For instructions we don't currently generate

#[derive(Clone, Copy, Debug)]
pub enum Riscv64Opnd
{
    // Dummy operand
    None,

    /*
    // Immediate value
    Imm(X86Imm),

    // Unsigned immediate
    UImm(X86UImm),
    */

    // General-purpose register
    Reg(Riscv64Reg),

    /*
    // Memory location
    Mem(X86Mem),

    // IP-relative memory location
    IPRel(i32)
    */
}

impl Riscv64Reg {
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Riscv64Reg
{
    // Size in bits
    pub num_bits: u8,

    // Register index number
    pub reg_no: u8,
}

impl Riscv64Reg {
  pub fn with_num_bits(&self, num_bits: u8) -> Self {
    assert!(
        num_bits == 8 ||
        num_bits == 16 ||
        num_bits == 32 ||
        num_bits == 64
    );
    Self {
        num_bits,
        reg_no: self.reg_no
    }
  }
}

pub const X0_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 0 };
pub const X1_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 1 };
pub const X2_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 2 };
pub const X3_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 3 };
pub const X4_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 4 };
pub const X5_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 5 };
pub const X6_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 6 };
pub const X7_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 7 };
pub const X8_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 8 };
pub const X9_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 9 };
pub const X10_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 10 };
pub const X11_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 11 };
pub const X12_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 12 };
pub const X13_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 13 };
pub const X14_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 14 };
pub const X15_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 15 };
pub const X16_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 16 };
pub const X17_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 17 };
pub const X18_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 18 };
pub const X19_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 19 };
pub const X20_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 20 };
pub const X21_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 21 };
pub const X22_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 22 };
pub const X23_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 23 };
pub const X24_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 24 };
pub const X25_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 25 };
pub const X26_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 26 };
pub const X27_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 27 };
pub const X28_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 28 };
pub const X29_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 29 };
pub const X30_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 30 };
pub const X31_REG: Riscv64Reg = Riscv64Reg { num_bits: 64, reg_no: 31 };

pub const RA_REG: Riscv64Reg = X1_REG;
pub const T0_REG: Riscv64Reg = X5_REG;
pub const T1_REG: Riscv64Reg = X6_REG;
pub const T2_REG: Riscv64Reg = X7_REG;
pub const A0_REG: Riscv64Reg = X10_REG;
pub const A1_REG: Riscv64Reg = X11_REG;
pub const A2_REG: Riscv64Reg = X12_REG;
pub const A3_REG: Riscv64Reg = X13_REG;
pub const A4_REG: Riscv64Reg = X14_REG;
pub const A5_REG: Riscv64Reg = X15_REG;
pub const A6_REG: Riscv64Reg = X16_REG;
pub const A7_REG: Riscv64Reg = X17_REG;
pub const S2_REG: Riscv64Reg = X18_REG;
pub const S3_REG: Riscv64Reg = X19_REG;
pub const S4_REG: Riscv64Reg = X20_REG;
pub const T3_REG: Riscv64Reg = X28_REG;
pub const T4_REG: Riscv64Reg = X29_REG;
pub const T5_REG: Riscv64Reg = X30_REG;
pub const T6_REG: Riscv64Reg = X31_REG;

