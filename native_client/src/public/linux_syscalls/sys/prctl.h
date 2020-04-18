/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_PUBLIC_LINUX_SYSCALLS_SYS_PRCTL_H_
#define NATIVE_CLIENT_SRC_PUBLIC_LINUX_SYSCALLS_SYS_PRCTL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int prctl(int option, ...);

#define PR_GET_DUMPABLE 3
#define PR_SET_DUMPABLE 4
#define PR_SET_NAME 15
#define PR_GET_NAME 16

#ifdef __cplusplus
}
#endif

#endif
