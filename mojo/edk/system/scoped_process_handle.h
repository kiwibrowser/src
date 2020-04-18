// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_SCOPED_PROCESS_HANDLE_H_
#define MOJO_EDK_SYSTEM_SCOPED_PROCESS_HANDLE_H_

#include "base/macros.h"
#include "base/process/process_handle.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include "base/win/scoped_handle.h"
#endif

namespace mojo {
namespace edk {

// Wraps a |base::ProcessHandle| with additional scoped lifetime semantics on
// applicable platforms. For platforms where process handles aren't ownable
// references, this is just a wrapper around |base::ProcessHandle|.
//
// This essentially exists to support passing around process handles internally
// in a generic way while also supporting Windows process handle ownership
// semantics.
class ScopedProcessHandle {
 public:
  ScopedProcessHandle();

  // Assumes ownership of |handle|.
  explicit ScopedProcessHandle(base::ProcessHandle handle);

  ScopedProcessHandle(ScopedProcessHandle&&);

  ~ScopedProcessHandle();

  // Creates a new ScopedProcessHandle from a clone of |handle|.
  static ScopedProcessHandle CloneFrom(base::ProcessHandle handle);

  ScopedProcessHandle& operator=(ScopedProcessHandle&&);

  bool is_valid() const {
#if defined(OS_WIN)
    return handle_.IsValid();
#else
    return handle_ != base::kNullProcessHandle;
#endif
  }

  base::ProcessHandle get() const {
#if defined(OS_WIN)
    return handle_.Get();
#else
    return handle_;
#endif
  }

  base::ProcessHandle release() {
#if defined(OS_WIN)
    return handle_.Take();
#else
    return handle_;
#endif
  }

  ScopedProcessHandle Clone() const;

 private:
#if defined(OS_WIN)
  base::win::ScopedHandle handle_;
#else
  base::ProcessHandle handle_ = base::kNullProcessHandle;
#endif

  DISALLOW_COPY_AND_ASSIGN(ScopedProcessHandle);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_SCOPED_PROCESS_HANDLE_H_
