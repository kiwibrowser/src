/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Basic Common Definitions.
 */
#ifndef NATIVE_CLIENT_SRC_INCLUDE_NACL_BASE_H_
#define NATIVE_CLIENT_SRC_INCLUDE_NACL_BASE_H_ 1

/*
 * The following part is necessary for Mips because Mips compilers by default
 * preprocess "mips" string and replace it with character '1'. To allow using
 * "NACL_mips" macro, we need to undefine "mips" macro.
 */

#ifdef mips
# undef mips
#endif

/*
 * putting extern "C" { } in header files make emacs want to indent
 * everything, which looks odd.  rather than putting in fancy syntax
 * recognition in c-mode, we just use the following macros.
 *
 * TODO: before releasing code, we should provide a defintion of a
 * function to be called from c-mode-hook that will make it easy to
 * follow our coding style (which we also need to document).
 */
#ifdef __cplusplus
# define EXTERN_C_BEGIN  extern "C" {
# define EXTERN_C_END    }
# if !defined(DISALLOW_COPY_AND_ASSIGN)
/*
 * This code is duplicated from base/basictypes.h, but including
 * that code should not be done except when building as part of Chrome.
 * Removing inclusion was necessitated by the fact that base/basictypes.h
 * sometimes defines CHECK, which conflicts with the NaCl definition.
 * Unfortunately this causes an include order dependency (this file has to
 * come after base/basictypes.h).
 * TODO(sehr): change CHECK to NACL_CHECK everywhere and remove this definition.
 */
#  define DISALLOW_COPY_AND_ASSIGN(TypeName) \
     TypeName(const TypeName&); \
     void operator=(const TypeName&)
#endif  /* !defined(DISALLOW_COPY_AND_ASSIGN) */

/* Mark this function as not throwing beyond */
# define NO_THROW throw()
#else
# define EXTERN_C_BEGIN
# define EXTERN_C_END
# define NO_THROW
#endif

/*
 * This is necessary to make "#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86" work.
 * #if-directives can work only with numerical values but not with strings e.g.
 * "NACL_x86"; therefore, we convert strings into integers. Whenever you use
 * NACL_ARCH or NACL_arm, you need to include this header.
 */
#define NACL_MERGE(x, y) x ## y
#define NACL_ARCH(x) NACL_MERGE(NACL_, x)
/*
 * Avoid using 0, because "#if FOO == 0" is true if FOO is undefined, and does
 * not produce a warning or error.
 */
#define NACL_x86  1
#define NACL_arm  2
#define NACL_mips 3
#define NACL_pnacl 4

/*****************************************************************************
 * Architecture name encodings.
 *
 * NACL_ARCH_NAME(name, arch) - Name specific to the given architecture,
 * NACL_SUBARCH_NAME(name, arch, subarch) - Name specific to the
 *    given architecture and subarchitecture.
 */
#define NACL_MERGE_ARCH_NAME(name, arch) NaCl_ ## name ## _ ## arch
#define NACL_ARCH_NAME(name, arch) NACL_MERGE_ARCH_NAME(name, arch)
#define NACL_MERGE_SUBARCH_NAME(name, arch, subarch) \
  NaCl_ ## name ## _ ## arch ## _ ## subarch
#define NACL_SUBARCH_NAME(name, arch, subarch) \
  NACL_MERGE_SUBARCH_NAME(name, arch, subarch)

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_NACL_BASE_H_ */
