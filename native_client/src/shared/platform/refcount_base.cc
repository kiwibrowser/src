/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/shared/platform/refcount_base.h"

#include "native_client/src/include/portability.h"  // NACL_PRIxPTR etc
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_check.h"

namespace nacl {

RefCountBase::RefCountBase() : refcount_(1) {
  if (!NaClMutexCtor(&mu_)) {
    NaClLog(LOG_FATAL, "scoped_ptr_refcount_obj: could not create mutex\n");
  }
}

RefCountBase* RefCountBase::Ref() {
  NaClXMutexLock(&mu_);
  if (0 == ++refcount_) {
    NaClLog(LOG_FATAL,
            ("scoped_ptr_refcount_obj: refcount overflow on 0x%08"
             NACL_PRIxPTR "\n"),
            reinterpret_cast<uintptr_t>(this));
  }
  NaClXMutexUnlock(&mu_);
  return this;
}

void RefCountBase::Unref() {
  NaClXMutexLock(&mu_);
  if (0 == refcount_) {
    NaClLog(LOG_FATAL,
            ("scoped_ptr_refcount_obj: Unref on zero refcount object: "
             "0x%08" NACL_PRIxPTR "\n"),
            reinterpret_cast<uintptr_t>(this));
  }
  bool do_delete = (0 == --refcount_);
  NaClXMutexUnlock(&mu_);
  if (do_delete) {
    delete this;
  }
}

RefCountBase::~RefCountBase() {
  CHECK(refcount_ == 0);
  NaClMutexDtor(&mu_);
  // Unlike in our src/trusted/nacl_base/nacl_refcount.h C code where
  // our ctor could fail and subclass ctors that fail must dtor the
  // base class object, in C++ the ctor must succeed.  Thus, the dtor
  // cannot possible encounter a count_==1 object, since that
  // situation only occurred when during ctor failure induced,
  // explicit dtor calls.
}

}  // namespace nacl
