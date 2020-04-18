/*
 * Copyright 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ARCH_MIPS_TRAMP_MIPS_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ARCH_MIPS_TRAMP_MIPS_H_

extern char NaCl_trampoline_seg_code;
extern char NaCl_trampoline_seg_end;
extern char NaCl_trampoline_syscall_seg_addr;

extern void NaClSyscallSeg(void);
extern void NaClSyscallSegEnd(void);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ARCH_MIPS_TRAMP_MIPS_H_ */
