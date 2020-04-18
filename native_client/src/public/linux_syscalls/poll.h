/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_PUBLIC_LINUX_SYSCALLS_INCLUDE_POLL_H_
#define NATIVE_CLIENT_SRC_PUBLIC_LINUX_SYSCALLS_INCLUDE_POLL_H_

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POLLIN  0x001
#define POLLOUT 0x004
#define POLLERR 0x008
#define POLLHUP 0x010

typedef uint32_t nfds_t;

struct pollfd {
  int32_t fd;
  int16_t events;
  int16_t revents;
};

int poll(struct pollfd *fds, nfds_t nfds, int timeout);

#ifdef __cplusplus
}
#endif

#endif
