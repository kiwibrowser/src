// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_MOVEABLE_AUTO_LOCK_H_
#define HEADLESS_PUBLIC_UTIL_MOVEABLE_AUTO_LOCK_H_

#include "base/synchronization/lock.h"

namespace headless {

class MoveableAutoLock {
 public:
  explicit MoveableAutoLock(base::Lock& lock) : lock_(lock), moved_(false) {
    lock_.Acquire();
  }

  MoveableAutoLock(MoveableAutoLock&& other)
      : lock_(other.lock_), moved_(other.moved_) {
    lock_.AssertAcquired();
    other.moved_ = true;
  }

  ~MoveableAutoLock() {
    if (moved_)
      return;
    lock_.AssertAcquired();
    lock_.Release();
  }

 private:
  base::Lock& lock_;
  bool moved_;
  DISALLOW_COPY_AND_ASSIGN(MoveableAutoLock);
};

// RAII helper to allow threadsafe access to an object guarded by a lock.
template <class T>
class LockedPtr {
 public:
  LockedPtr(MoveableAutoLock&& lock, T* object)
      : lock_(std::move(lock)), object_(object) {}

  LockedPtr(LockedPtr&& other)
      : lock_(std::move(other.lock_)), object_(other.object_) {}

  T* operator->() { return object_; }

  T& operator*() { return *object_; }

  T* Get() { return object_; }

  explicit operator bool() const { return object_; }

 private:
  MoveableAutoLock lock_;
  T* object_;

  DISALLOW_COPY_AND_ASSIGN(LockedPtr);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_MOVEABLE_AUTO_LOCK_H_
