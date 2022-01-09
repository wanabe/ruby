#if YJIT_TARGET_ARCH == YJIT_ARCH_X86_64
# include "yjit_utils_x86_64.c"
#elif YJIT_TARGET_ARCH == YJIT_ARCH_ARM64
# include "yjit_utils_arm64.c"
#endif
