/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl semaphore type (Windows)
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_NACL_SEMAPHORE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_NACL_SEMAPHORE_H_

#define SEM_VALUE_MAX (2147483647)

typedef long NTSTATUS;

struct NaClSemaphore {
  HANDLE sem_handle;
  HANDLE interrupt_event;
};

void NaClSemIntr(struct NaClSemaphore *sem);

void NaClSemReset(struct NaClSemaphore *sem);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_NACL_SEMAPHORE_H_ */
