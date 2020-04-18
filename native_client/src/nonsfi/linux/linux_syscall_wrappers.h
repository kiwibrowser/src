/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_NONSFI_LINUX_LINUX_SYSCALL_WRAPPERS_H_
#define NATIVE_CLIENT_SRC_NONSFI_LINUX_LINUX_SYSCALL_WRAPPERS_H_ 1

/*
 * Wrappers for calling Linux syscalls directly.
 *
 * This provides functionality similar to linux_syscall_support.h (from
 * https://code.google.com/p/linux-syscall-support/), also known as LSS.
 * We are not reusing LSS here because:
 *
 *  * LSS depends on Linux kernel headers (and glibc headers, to a lesser
 *    extent).  We would have to import these into the NaCl build and
 *    resolve conflicts that they have with nacl-newlib's headers.
 *
 *  * We have peculiar requirements such as needing to execute syscalls
 *    from specific program counter addresses in memory, in order to
 *    whitelist these addresses in a seccomp-bpf filter.  LSS's
 *    SYS_SYSCALL_ENTRYPOINT is more complex than what we need, and also
 *    won't work with clone().
 *
 *  * LSS lacks a test suite, so even if we got it working with the
 *    nacl-newlib toolchain, this usage might easily be broken by future
 *    changes.
 */

#include <stdint.h>

#if defined(__i386__)

/*
 * Registers used for system call parameters on x86-32:
 *   %eax - system call number and return value
 *   %ebx - argument 1, preserved
 *   %ecx - argument 2, preserved
 *   %edx - argument 3, preserved
 *   %esi - argument 4, preserved
 *   %edi - argument 5, preserved
 *   %ebp - argument 6, preserved
 */

static inline uint32_t linux_syscall0(int syscall_number) {
  uint32_t result;
  __asm__ __volatile__("int $0x80\n"
                       : "=a"(result)
                       : "a"(syscall_number));
  return result;
}

static inline uint32_t linux_syscall1(int syscall_number, uint32_t arg1) {
  uint32_t result;
  __asm__ __volatile__("int $0x80\n"
                       : "=a"(result)
                       : "a"(syscall_number), "b"(arg1));
  return result;
}

static inline uint32_t linux_syscall2(int syscall_number,
                                      uint32_t arg1, uint32_t arg2) {
  uint32_t result;
  __asm__ __volatile__("int $0x80\n"
                       : "=a"(result)
                       : "a"(syscall_number), "b"(arg1), "c"(arg2));
  return result;
}

static inline uint32_t linux_syscall3(int syscall_number,
                                      uint32_t arg1, uint32_t arg2,
                                      uint32_t arg3) {
  uint32_t result;
  __asm__ __volatile__("int $0x80\n"
                       : "=a"(result)
                       : "a"(syscall_number), "b"(arg1), "c"(arg2), "d"(arg3));
  return result;
}

static inline uint32_t linux_syscall4(int syscall_number,
                                      uint32_t arg1, uint32_t arg2,
                                      uint32_t arg3, uint32_t arg4) {
  uint32_t result;
  __asm__ __volatile__("int $0x80\n"
                       : "=a"(result)
                       : "a"(syscall_number), "b"(arg1), "c"(arg2), "d"(arg3),
                         "S"(arg4));
  return result;
}

static inline uint32_t linux_syscall5(int syscall_number,
                                      uint32_t arg1, uint32_t arg2,
                                      uint32_t arg3, uint32_t arg4,
                                      uint32_t arg5) {
  uint32_t result;
  __asm__ __volatile__("int $0x80\n"
                       : "=a"(result)
                       : "a"(syscall_number), "b"(arg1), "c"(arg2), "d"(arg3),
                         "S"(arg4), "D"(arg5));
  return result;
}

static inline uint32_t linux_syscall6(int syscall_number,
                                      uint32_t arg1, uint32_t arg2,
                                      uint32_t arg3, uint32_t arg4,
                                      uint32_t arg5, uint32_t arg6) {
  /*
   * Inline assembly doesn't let us use a constraint to set %ebp, which is
   * the 6th syscall argument on x86-32.  To set %ebp, we use the trick of
   * saving some registers on the stack.
   *
   * Note, however, that this means unwind info will be incorrect during
   * this syscall.
   */
  uint32_t args[2] = { arg1, arg6 };
  uint32_t result;
  __asm__ __volatile__("push %%ebp\n"
                       "movl 4(%%ebx), %%ebp\n" /* arg6 */
                       "movl 0(%%ebx), %%ebx\n" /* arg1 */
                       "int $0x80\n"
                       "pop %%ebp\n"
                       : "=a"(result)
                       : "a"(syscall_number), "b"(&args),
                         "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5)
                       : "ebx", "memory");
  return result;
}

#elif defined(__arm__)

/*
 * Registers used for system call parameters on ARM EABI:
 *   r7 - system call number
 *   r0 - argument 1 and return value
 *   r1 - argument 2, preserved
 *   r2 - argument 3, preserved
 *   r3 - argument 4, preserved
 *   r4 - argument 5, preserved
 *   r5 - argument 6, preserved
 */

static inline uint32_t linux_syscall0(int syscall_number) {
  register uint32_t sysno __asm__("r7") = syscall_number;
  register uint32_t result __asm__("r0");
  __asm__ __volatile__("svc #0\n"
                       : "=r"(result)
                       : "r"(sysno)
                       : "memory");
  return result;
}

static inline uint32_t linux_syscall1(int syscall_number, uint32_t arg1) {
  register uint32_t sysno __asm__("r7") = syscall_number;
  register uint32_t a1 __asm__("r0") = arg1;
  register uint32_t result __asm__("r0");
  __asm__ __volatile__("svc #0\n"
                       : "=r"(result)
                       : "r"(sysno), "r"(a1)
                       : "memory");
  return result;
}

static inline uint32_t linux_syscall2(int syscall_number,
                                      uint32_t arg1, uint32_t arg2) {
  register uint32_t sysno __asm__("r7") = syscall_number;
  register uint32_t a1 __asm__("r0") = arg1;
  register uint32_t a2 __asm__("r1") = arg2;
  register uint32_t result __asm__("r0");
  __asm__ __volatile__("svc #0\n"
                       : "=r"(result)
                       : "r"(sysno), "r"(a1), "r"(a2)
                       : "memory");
  return result;
}

static inline uint32_t linux_syscall3(int syscall_number,
                                      uint32_t arg1, uint32_t arg2,
                                      uint32_t arg3) {
  register uint32_t sysno __asm__("r7") = syscall_number;
  register uint32_t a1 __asm__("r0") = arg1;
  register uint32_t a2 __asm__("r1") = arg2;
  register uint32_t a3 __asm__("r2") = arg3;
  register uint32_t result __asm__("r0");
  __asm__ __volatile__("svc #0\n"
                       : "=r"(result)
                       : "r"(sysno), "r"(a1), "r"(a2), "r"(a3)
                       : "memory");
  return result;
}

static inline uint32_t linux_syscall4(int syscall_number,
                                      uint32_t arg1, uint32_t arg2,
                                      uint32_t arg3, uint32_t arg4) {
  register uint32_t sysno __asm__("r7") = syscall_number;
  register uint32_t a1 __asm__("r0") = arg1;
  register uint32_t a2 __asm__("r1") = arg2;
  register uint32_t a3 __asm__("r2") = arg3;
  register uint32_t a4 __asm__("r3") = arg4;
  register uint32_t result __asm__("r0");
  __asm__ __volatile__("svc #0\n"
                       : "=r"(result)
                       : "r"(sysno),
                         "r"(a1), "r"(a2), "r"(a3), "r"(a4)
                       : "memory");
  return result;
}

static inline uint32_t linux_syscall5(int syscall_number,
                                      uint32_t arg1, uint32_t arg2,
                                      uint32_t arg3, uint32_t arg4,
                                      uint32_t arg5) {
  register uint32_t sysno __asm__("r7") = syscall_number;
  register uint32_t a1 __asm__("r0") = arg1;
  register uint32_t a2 __asm__("r1") = arg2;
  register uint32_t a3 __asm__("r2") = arg3;
  register uint32_t a4 __asm__("r3") = arg4;
  register uint32_t a5 __asm__("r4") = arg5;
  register uint32_t result __asm__("r0");
  __asm__ __volatile__("svc #0\n"
                       : "=r"(result)
                       : "r"(sysno),
                         "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5)
                       : "memory");
  return result;
}

static inline uint32_t linux_syscall6(int syscall_number,
                                      uint32_t arg1, uint32_t arg2,
                                      uint32_t arg3, uint32_t arg4,
                                      uint32_t arg5, uint32_t arg6) {
  register uint32_t sysno __asm__("r7") = syscall_number;
  register uint32_t a1 __asm__("r0") = arg1;
  register uint32_t a2 __asm__("r1") = arg2;
  register uint32_t a3 __asm__("r2") = arg3;
  register uint32_t a4 __asm__("r3") = arg4;
  register uint32_t a5 __asm__("r4") = arg5;
  register uint32_t a6 __asm__("r5") = arg6;
  register uint32_t result __asm__("r0");
  __asm__ __volatile__("svc #0\n"
                       : "=r"(result)
                       : "r"(sysno),
                         "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6)
                       : "memory");
  return result;
}

#else
# error Unsupported architecture
#endif

static inline int linux_is_error_result(uint32_t result) {
  /*
   * -0x1000 is the highest address that mmap() can return as a result.
   * Linux errno values are less than 0x1000.
   */
  return result > (uint32_t) -0x1000;
}

#endif
