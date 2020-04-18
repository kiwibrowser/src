/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_NACL_INCLUDE_SYS_UTSNAME_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_NACL_INCLUDE_SYS_UTSNAME_H_

#define _UTSNAME_LENGTH 65

struct utsname {
  char sysname[_UTSNAME_LENGTH];
  char nodename[_UTSNAME_LENGTH];
  char release[_UTSNAME_LENGTH];
  char version[_UTSNAME_LENGTH];
  char machine[_UTSNAME_LENGTH];
};

#if defined(__cplusplus)
extern "C" {
#endif

int uname(struct utsname *name);

#if defined(__cplusplus)
}
#endif

#endif  /* NATIVE_CLIENT_SRC_UNTRUSTED_NACL_INCLUDE_SYS_UTSNAME_H_ */
