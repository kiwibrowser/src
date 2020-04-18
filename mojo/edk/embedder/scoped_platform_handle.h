// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_SCOPED_PLATFORM_HANDLE_H_
#define MOJO_EDK_EMBEDDER_SCOPED_PLATFORM_HANDLE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "mojo/edk/embedder/platform_handle.h"
#include "mojo/edk/system/system_impl_export.h"
#include "mojo/public/c/system/macros.h"

namespace mojo {
namespace edk {

class MOJO_SYSTEM_IMPL_EXPORT ScopedInternalPlatformHandle {
 public:
  ScopedInternalPlatformHandle() {}
  explicit ScopedInternalPlatformHandle(InternalPlatformHandle handle)
      : handle_(handle) {}
  ~ScopedInternalPlatformHandle() { handle_.CloseIfNecessary(); }

  // Move-only constructor and operator=.
  ScopedInternalPlatformHandle(ScopedInternalPlatformHandle&& other)
      : handle_(other.release()) {}

  ScopedInternalPlatformHandle& operator=(
      ScopedInternalPlatformHandle&& other) {
    reset(other.release());
    return *this;
  }

  const InternalPlatformHandle& get() const { return handle_; }
  InternalPlatformHandle& get() { return handle_; }

  void swap(ScopedInternalPlatformHandle& other) {
    InternalPlatformHandle temp = handle_;
    handle_ = other.handle_;
    other.handle_ = temp;
  }

  InternalPlatformHandle release() WARN_UNUSED_RESULT {
    InternalPlatformHandle rv = handle_;
    handle_ = InternalPlatformHandle();
    return rv;
  }

  void reset(InternalPlatformHandle handle = InternalPlatformHandle()) {
    handle_.CloseIfNecessary();
    handle_ = handle;
  }

  bool is_valid() const { return handle_.is_valid(); }

 private:
  InternalPlatformHandle handle_;

  DISALLOW_COPY_AND_ASSIGN(ScopedInternalPlatformHandle);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_SCOPED_PLATFORM_HANDLE_H_
