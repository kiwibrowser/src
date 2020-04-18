/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ARCH_ARM_TRAMP_ARM_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ARCH_ARM_TRAMP_ARM_H_

extern const char NaCl_trampoline_seg_code;
extern const char NaCl_trampoline_seg_end;
extern const char NaCl_trampoline_syscall_seg_addr;

extern void NaClSyscallSeg(void);
extern void NaClSyscallSegRegsSaved(void);
extern void NaClSyscallSegEnd(void);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ARCH_ARM_TRAMP_ARM_H_ */
