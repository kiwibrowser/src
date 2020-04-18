// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/named_platform_handle_utils.h"

#include "mojo/edk/embedder/named_platform_handle.h"

namespace mojo {
namespace edk {

ScopedInternalPlatformHandle CreateClientHandle(
    const NamedPlatformHandle& named_handle) {
  // TODO(fuchsia): Implement, or remove dependencies (crbug.com/754038).
  NOTREACHED();
  return ScopedInternalPlatformHandle();
}

ScopedInternalPlatformHandle CreateServerHandle(
    const NamedPlatformHandle& named_handle,
    const CreateServerHandleOptions& options) {
  // TODO(fuchsia): Implement, or remove dependencies (crbug.com/754038).
  NOTREACHED();
  return ScopedInternalPlatformHandle();
}

}  // namespace edk
}  // namespace mojo
