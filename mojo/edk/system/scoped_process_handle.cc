// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/scoped_process_handle.h"

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

namespace mojo {
namespace edk {

ScopedProcessHandle::ScopedProcessHandle() = default;

ScopedProcessHandle::ScopedProcessHandle(base::ProcessHandle handle)
    : handle_(handle) {}

ScopedProcessHandle::ScopedProcessHandle(ScopedProcessHandle&&) = default;

ScopedProcessHandle::~ScopedProcessHandle() = default;

// static
ScopedProcessHandle ScopedProcessHandle::CloneFrom(base::ProcessHandle handle) {
#if defined(OS_WIN)
  BOOL ok = ::DuplicateHandle(base::GetCurrentProcessHandle(), handle,
                              base::GetCurrentProcessHandle(), &handle, 0,
                              FALSE, DUPLICATE_SAME_ACCESS);
  DCHECK(ok);
#endif
  return ScopedProcessHandle(handle);
}

ScopedProcessHandle& ScopedProcessHandle::operator=(ScopedProcessHandle&&) =
    default;

ScopedProcessHandle ScopedProcessHandle::Clone() const {
  DCHECK(is_valid());
  return CloneFrom(get());
}

}  // namespace edk
}  // namespace mojo
