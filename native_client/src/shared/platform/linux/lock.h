/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_LOCK_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_LOCK_H_

#include <pthread.h>
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_sync.h"

// This class implements the underlying platform-specific spin-lock mechanism
// used for the Lock class.  Most users should not use LockImpl directly, but
// should instead use Lock.

namespace NaCl {

// A helper class that acquires the given Lock while the AutoLock is in scope.
class AutoLock {
 public:
  explicit AutoLock(Lock& lock) : lock_(lock) {
    lock_.Acquire();
  }

  ~AutoLock() {
    lock_.Release();
  }

 private:
  Lock& lock_;
  NACL_DISALLOW_COPY_AND_ASSIGN(AutoLock);
};

// A helper macro to perform a single operation (expressed by expr)
// in a lock
#define LOCKED_EXPRESSION(lock, expr) \
  do { \
    NaCl::AutoLock _auto_lock(lock);  \
    (expr); \
  } while (0)

}  // namespace NaCl

#endif  // NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_LOCK_H_
