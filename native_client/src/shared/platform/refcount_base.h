/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_SHARED_PLATFORM_REFCOUNT_BASE_H_
#define NATIVE_CLIENT_SRC_SHARED_PLATFORM_REFCOUNT_BASE_H_

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

// syntactic glucose to refcount arbitrary object using platform
// library's synchronization primitives.

namespace nacl {
//
// base class for refcounted objects
//
class RefCountBase {
 public:
  RefCountBase();

  RefCountBase* Ref();
  // subclasses probably want co-variant overloads

  void Unref();

 protected:
  virtual ~RefCountBase();
  // dtor (or one method) must be virtual to ensure that the base
  // class has a vtbl pointer, so that the covariant casts of Ref()
  // using reinterpret_cast<SubClass>(RefCountBase::Ref()) in
  // subclasses that have virtual functions

 private:
  NaClMutex mu_;
  uint32_t refcount_;

  NACL_DISALLOW_COPY_AND_ASSIGN(RefCountBase);
};

}  // namespace

#endif
