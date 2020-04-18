// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_MULTI_PROCESS_LOCK_H_
#define CHROME_COMMON_MULTI_PROCESS_LOCK_H_

#include <sys/types.h>
#include <string>

// Platform abstraction for a lock that can be shared between processes.
// The process that owns the lock will release it on exit even if
// the exit is due to a crash. Locks are not recursive.
class MultiProcessLock {
 public:

  // Factory method for creating a multi-process lock.
  // |name| is the name of the lock. The name has special meaning on Windows
  // where the prefix can determine the namespace of the lock.
  // See http://msdn.microsoft.com/en-us/library/aa382954(v=VS.85).aspx for
  // details.
  static MultiProcessLock* Create(const std::string& name);

  virtual ~MultiProcessLock() { }

  // Try to grab ownership of the lock.
  virtual bool TryLock() = 0;

  // Release ownership of the lock.
  virtual void Unlock() = 0;
};

#endif  // CHROME_COMMON_MULTI_PROCESS_LOCK_H_
