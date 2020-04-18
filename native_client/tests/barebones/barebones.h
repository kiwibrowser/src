/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * these were lifted from src/trusted/service_runtime/nacl_config.h
 * NOTE: we cannot include this file here
 * TODO(robertm): make this file available in the sdk
 * http://code.google.com/p/nativeclient/issues/detail?id=386
 */

#ifndef BAREBONES_H_
#define BAREBONES_H_

#define NACL_INSTR_BLOCK_SHIFT         5
#define NACL_PAGESHIFT                12
#define NACL_SYSCALL_START_ADDR       (16 << NACL_PAGESHIFT)
#define NACL_SYSCALL_ADDR(syscall_number)                               \
     (NACL_SYSCALL_START_ADDR + (syscall_number << NACL_INSTR_BLOCK_SHIFT))

#define NACL_SYSCALL(s) ((TYPE_nacl_ ## s) NACL_SYSCALL_ADDR(NACL_sys_ ## s))

typedef int (*TYPE_nacl_write) (int desc, void const *buf, int count);
typedef void (*TYPE_nacl_null) (void);
typedef void (*TYPE_nacl_exit) (int status);

#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"


#define myprint(s) NACL_SYSCALL(write)(1, s, mystrlen(s))

#define THIS_IS_ALWAYS_FALSE_FOR_SMALL_NUMBERS(n) \
  (n * n - 1 != (n + 1) * (n - 1))

int mystrlen(const char* s) {
  int count = 0;
  while (*s++) ++count;
  return count;
}


void myhextochar(int n, char buffer[9]) {
  int i;
  buffer[8] = 0;

  for (i = 0; i < 8; ++i) {
    int nibble = 0xf & (n >> (4 * (7 - i)));
    if (nibble <= 9) {
      buffer[i] = nibble + '0';
    } else {
      buffer[i] = nibble - 10 + 'A';
    }
  }
}

#endif  /* BAREBONES_H_ */
