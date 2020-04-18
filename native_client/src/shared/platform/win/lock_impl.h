/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_LOCK_IMPL_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_LOCK_IMPL_H_

#include <Windows.h>
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_sync.h"

namespace NaCl {

// This class implements the underlying platform-specific spin-lock mechanism
// used for the Lock class.  Most users should not use LockImpl directly, but
// should instead use Lock.
class LockImpl {
 public:
  LockImpl();
  ~LockImpl();

  // If the lock is not held, take it and return true.  If the lock is already
  // held by something else, immediately return false.
  bool Try();

  // Take the lock, blocking until it is available if necessary.
  void Lock();

  // Release the lock.  This must only be called by the lock's holder: after
  // a successful call to Try, or a call to Lock.
  void Unlock();

 private:
  CRITICAL_SECTION mu_;

  NACL_DISALLOW_COPY_AND_ASSIGN(LockImpl);
};

class AutoLockImpl {
 public:
  explicit AutoLockImpl(LockImpl* lock_impl)
      : lock_impl_(lock_impl) {
    lock_impl_->Lock();
  }

  ~AutoLockImpl() {
    lock_impl_->Unlock();
  }

 private:
  LockImpl* lock_impl_;
  NACL_DISALLOW_COPY_AND_ASSIGN(AutoLockImpl);
};

}  // namespace NaCl

#endif  // NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_LOCK_IMPL_H_
