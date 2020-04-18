// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TRIVIAL_CTOR_H_
#define TRIVIAL_CTOR_H_

// Mocked for testing:
namespace std {

template<typename T>
struct atomic {
  T i;
};

typedef atomic<int> atomic_int;

}  // namespace std

struct MySpinLock {
  MySpinLock();
  ~MySpinLock();
  MySpinLock(const MySpinLock&);
  MySpinLock(MySpinLock&&);
  std::atomic_int lock_;
};

#endif  // TRIVIAL_CTOR_H_
