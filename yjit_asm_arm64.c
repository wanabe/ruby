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

#include "yjit_asm.h"
#include "yjit.h"

arm64opnd_t mem_opnd(uint32_t num_bits, arm64opnd_t base_reg, int32_t disp)
{
    // bool is_iprel = base_reg.as.reg.reg_type == REG_IP;

    arm64opnd_t opnd = {
        OPND_MEM,
        num_bits,
        //.as.mem = { base_reg.as.reg.reg_no, 0, 0, false, is_iprel, disp }
    };

    return opnd;
}

