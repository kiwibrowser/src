// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_PLATFORM_HANDLE_UTILS_H_
#define MOJO_EDK_EMBEDDER_PLATFORM_HANDLE_UTILS_H_

#include "base/memory/platform_shared_memory_region.h"
#include "mojo/edk/embedder/platform_handle.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/system/system_impl_export.h"
#include "mojo/public/c/system/platform_handle.h"
#include "mojo/public/c/system/types.h"
#include "mojo/public/cpp/platform/platform_handle.h"

namespace mojo {
namespace edk {

// Closes all the |InternalPlatformHandle|s in the given container.
template <typename InternalPlatformHandleContainer>
MOJO_SYSTEM_IMPL_EXPORT inline void CloseAllInternalPlatformHandles(
    InternalPlatformHandleContainer* platform_handles) {
  for (typename InternalPlatformHandleContainer::iterator it =
           platform_handles->begin();
       it != platform_handles->end(); ++it)
    it->CloseIfNecessary();
}

MOJO_SYSTEM_IMPL_EXPORT MojoResult
MojoPlatformHandleToScopedInternalPlatformHandle(
    const MojoPlatformHandle* platform_handle,
    ScopedInternalPlatformHandle* out_handle);

MOJO_SYSTEM_IMPL_EXPORT MojoResult
ScopedInternalPlatformHandleToMojoPlatformHandle(
    ScopedInternalPlatformHandle handle,
    MojoPlatformHandle* platform_handle);

// Duplicates the given |InternalPlatformHandle| (which must be valid). (Returns
// an invalid |ScopedInternalPlatformHandle| on failure.)
MOJO_SYSTEM_IMPL_EXPORT ScopedInternalPlatformHandle
DuplicatePlatformHandle(InternalPlatformHandle platform_handle);

// Converts a base shared memory platform handle into one (maybe two on POSIX)
// EDK ScopedInternalPlatformHandles.
MOJO_SYSTEM_IMPL_EXPORT void
ExtractInternalPlatformHandlesFromSharedMemoryRegionHandle(
    base::subtle::PlatformSharedMemoryRegion::ScopedPlatformHandle handle,
    ScopedInternalPlatformHandle* extracted_handle,
    ScopedInternalPlatformHandle* extracted_readonly_handle);

// Converts one (maybe two on POSIX) EDK ScopedInternalPlatformHandles to a base
// shared memory platform handle.
MOJO_SYSTEM_IMPL_EXPORT
base::subtle::PlatformSharedMemoryRegion::ScopedPlatformHandle
CreateSharedMemoryRegionHandleFromInternalPlatformHandles(
    ScopedInternalPlatformHandle handle,
    ScopedInternalPlatformHandle readonly_handle);

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_PLATFORM_HANDLE_UTILS_H_
