/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_PUBLIC_LINUX_SYSCALLS_SYS_RESOURCE_H_
#define NATIVE_CLIENT_SRC_PUBLIC_LINUX_SYSCALLS_SYS_RESOURCE_H_

/*
 * Here, we declare this is a system header by #pragma directive to disable
 * warning.
 */
#pragma GCC system_header
#include_next <sys/resource.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PRIO_PROCESS 0

#define RLIMIT_DATA 2
#define RLIMIT_CORE 4
#define RLIMIT_NOFILE 7
#define RLIMIT_AS 9

typedef uint32_t rlim_t;

struct rlimit {
  rlim_t rlim_cur;
  rlim_t rlim_max;
};

int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);

#ifdef __cplusplus
}
#endif

#endif
