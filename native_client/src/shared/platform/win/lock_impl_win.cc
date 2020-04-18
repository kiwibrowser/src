/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <io.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/win/lock_impl.h"

NaCl::LockImpl::LockImpl() {
  InitializeCriticalSection(&mu_);
}

NaCl::LockImpl::~LockImpl() {
  DeleteCriticalSection(&mu_);
}

bool NaCl::LockImpl::Try() {
  return TryEnterCriticalSection(&mu_) != 0;
}

void NaCl::LockImpl::Lock() {
  EnterCriticalSection(&mu_);
}

void NaCl::LockImpl::Unlock() {
  LeaveCriticalSection(&mu_);
}
