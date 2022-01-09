#ifndef YJIT_REG_H
#define YJIT_REG_H 1

#include "yjit.h"

#if YJIT_TARGET_ARCH == YJIT_ARCH_X86_64
# include "yjit_reg_x86_64.h"
#endif

#endif // #ifndef YJIT_REG_H
