// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/platform_handle_utils.h"

#include "base/logging.h"

namespace mojo {
namespace edk {

ScopedInternalPlatformHandle DuplicatePlatformHandle(
    InternalPlatformHandle platform_handle) {
  DCHECK(platform_handle.is_valid());
  zx_handle_t duped;
  // zx_handle_duplicate won't touch |duped| in case of failure.
  zx_status_t result = zx_handle_duplicate(platform_handle.as_handle(),
                                           ZX_RIGHT_SAME_RIGHTS, &duped);
  DLOG_IF(ERROR, result != ZX_OK) << "zx_duplicate_handle failed: " << result;
  return ScopedInternalPlatformHandle(InternalPlatformHandle::ForHandle(duped));
}

}  // namespace edk
}  // namespace mojo
