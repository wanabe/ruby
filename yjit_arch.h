#ifndef YJIT_ARCH_H
#define YJIT_ARCH_H 1

#define YJIT_ARCH_NONE 0
#define YJIT_ARCH_X86_64 1
#define YJIT_ARCH_ARM64 2

#ifndef YJIT_TARGET_ARCH
// We generate x86 assembly
# if (defined(__x86_64__) && !defined(_WIN32)) || (defined(_WIN32) && defined(_M_AMD64)) // x64 platforms without mingw/msys
#  define YJIT_TARGET_ARCH YJIT_ARCH_X86_64
# elif defined(__aarch64__)
#  define YJIT_TARGET_ARCH YJIT_ARCH_ARM64
# endif
#endif

#endif // #ifndef YJIT_ARCH_H