/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_PORT_MUTEX_H_
#define NATIVE_CLIENT_PORT_MUTEX_H_ 1

#include <assert.h>
#include <stddef.h>

#include "native_client/src/shared/platform/nacl_sync_checked.h"

namespace port {


// MutexLock
//   A MutexLock object will lock on construction and automatically
// unlock on destruction of the object as the object goes out of scope.
class MutexLock {
 public:
  explicit MutexLock(struct NaClMutex *mutex) : mutex_(mutex) {
    NaClXMutexLock(mutex_);
  }
  ~MutexLock() {
    NaClXMutexUnlock(mutex_);
  }

 private:
  struct NaClMutex *mutex_;
  MutexLock(const MutexLock&);
  MutexLock &operator=(const MutexLock&);
};


}  // namespace port

#endif  // NATIVE_CLIENT_PORT_MUTEX_H_

