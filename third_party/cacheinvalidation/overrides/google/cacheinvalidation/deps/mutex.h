// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_CACHEINVALIDATION_DEPS_MUTEX_H_
#define GOOGLE_CACHEINVALIDATION_DEPS_MUTEX_H_

#include "base/macros.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"

namespace invalidation {

typedef base::Lock Mutex;

class MutexLock {
 public:
  explicit MutexLock(Mutex* m) : auto_lock_(*m) {}

 private:
  base::AutoLock auto_lock_;

  DISALLOW_COPY_AND_ASSIGN(MutexLock);
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_DEPS_MUTEX_H_
