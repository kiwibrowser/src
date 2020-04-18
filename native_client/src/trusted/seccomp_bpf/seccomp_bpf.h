/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SECCOMP_BPF_SECCOMP_BPF_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SECCOMP_BPF_SECCOMP_BPF_H_ 1

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Turns on seccomp-bpf syscall policy.
 * On success, returns 0.
 */
int NaClInstallBpfFilter(void);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SECCOMP_BPF_SECCOMP_BPF_H_ */
