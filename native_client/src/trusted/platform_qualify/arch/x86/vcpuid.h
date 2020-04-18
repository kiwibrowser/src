/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_ARCH_X86_VCPUID_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_ARCH_X86_VCPUID_H_
/*
 * vcpuid.h
 *
 * Verify correctness of CPUID implementation.
 *
 * This uses shell status code to indicate its result; non-zero return
 * code indicates the CPUID instruction is not implemented or not
 * implemented correctly.
 */

/* This routine routines zero if CPUID appears to be implemented correctly,
 * and otherwise non-zero.
 */
int CPUIDImplIsValid(void);
#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_ARCH_X86_VCPUID_H_ */
