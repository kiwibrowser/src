/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_PUBLIC_LINUX_SYSCALLS_SCHED_H_
#define NATIVE_CLIENT_SRC_PUBLIC_LINUX_SYSCALLS_SCHED_H_ 1

/*
 * Here, we declare this is a system header by #pragma directive to disable
 * warning.
 */
#pragma GCC system_header
#include_next <sched.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLONE_VM             0x00000100
#define CLONE_FS             0x00000200
#define CLONE_FILES          0x00000400
#define CLONE_SIGHAND        0x00000800
#define CLONE_VFORK          0x00004000
#define CLONE_THREAD         0x00010000
#define CLONE_NEWNS          0x00020000
#define CLONE_SYSVSEM        0x00040000
#define CLONE_SETTLS         0x00080000
#define CLONE_PARENT_SETTID  0x00100000
#define CLONE_CHILD_CLEARTID 0x00200000
#define CLONE_DETACHED       0x00400000
#define CLONE_CHILD_SETTID   0x01000000
#define CLONE_NEWUTS         0x04000000
#define CLONE_NEWIPC         0x08000000
#define CLONE_NEWUSER        0x10000000
#define CLONE_NEWPID         0x20000000
#define CLONE_NEWNET         0x40000000

int clone(int (*fn)(void *), void *child_stack, int flags, void *arg, ...);

#ifdef __cplusplus
}
#endif

#endif
