/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _NATIVE_CLIENT_SRC_UNTRUSTED_NACL_PNACLINTRIN_H_
#define _NATIVE_CLIENT_SRC_UNTRUSTED_NACL_PNACLINTRIN_H_ 1

#if defined(__cplusplus)
extern "C" {
#endif

/* Enumeration values returned by __nacl_get_arch(). */
enum PnaclTargetArchitecture {
  PnaclTargetArchitectureInvalid = 0,
  PnaclTargetArchitectureX86_32,
  PnaclTargetArchitectureX86_32_NonSFI,
  PnaclTargetArchitectureX86_64,
  PnaclTargetArchitectureARM_32,
  PnaclTargetArchitectureARM_32_NonSFI,
  PnaclTargetArchitectureARM_32_Thumb,
  PnaclTargetArchitectureMips_32
};

#if defined(NACL_DEFINE_EXTERNAL_NATIVE_SUPPORT_FUNCS)
# define NACL_GET_ARCH_FUNC
#else
# define NACL_GET_ARCH_FUNC static inline
#endif

#if defined(__native_client_nonsfi__)
#if defined(__i386__)
NACL_GET_ARCH_FUNC int __nacl_get_arch(void) {
  return PnaclTargetArchitectureX86_32_NonSFI;
}
#elif defined(__arm__)
NACL_GET_ARCH_FUNC int __nacl_get_arch(void) {
  return PnaclTargetArchitectureARM_32_NonSFI;
}
#else
# error "Unknown architecture for __nacl_get_arch()"
#endif
#else /* __native_client_nonsfi__ */
#if defined(__i386__)
NACL_GET_ARCH_FUNC int __nacl_get_arch(void) {
  return PnaclTargetArchitectureX86_32;
}
#elif defined(__x86_64__)
NACL_GET_ARCH_FUNC int __nacl_get_arch(void) {
  return PnaclTargetArchitectureX86_64;
}
#elif defined(__arm__)
NACL_GET_ARCH_FUNC int __nacl_get_arch(void) {
  return PnaclTargetArchitectureARM_32;
}
#elif defined(__mips__)
NACL_GET_ARCH_FUNC int __nacl_get_arch(void) {
  return PnaclTargetArchitectureMips_32;
}
#elif defined(__pnacl__)
/*
 * This is defined by PNaCl's native support code, but it is not
 * available to ABI-stable pexes.
 */
int __nacl_get_arch(void);
#else
# error "Unknown architecture for __nacl_get_arch()"
#endif
#endif /* __native_client_nonsfi__ */

#undef NACL_GET_ARCH_FUNC

/*
 * This is a deprecated alias for __nacl_get_arch().  The name is
 * misleading because this is not a compiler builtin.
 * TODO(mseaborn): Remove this when the uses in pnacl-llvm are removed.
 */
static inline int __builtin_nacl_target_arch(void) {
  return __nacl_get_arch();
}

#if defined(__cplusplus)
}
#endif

#endif /* _NATIVE_CLIENT_SRC_UNTRUSTED_NACL_PNACLINTRIN_H_ */
