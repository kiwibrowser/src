// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_BITMAP_ALLOCATION_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_BITMAP_ALLOCATION_H_

#include <memory>

#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/viz_common_export.h"
#include "mojo/public/cpp/system/buffer.h"

namespace base {
class SharedMemory;
}

namespace gfx {
class Size;
}  // namespace gfx

namespace viz {

namespace bitmap_allocation {

// Allocates a shared memory segment to hold |size| pixels in RGBA_8888
// format. Crashes if allocation does not succeed. The returned SharedMemory
// will be mapped.
VIZ_COMMON_EXPORT std::unique_ptr<base::SharedMemory> AllocateMappedBitmap(
    const gfx::Size& size,
    ResourceFormat format);

// For a bitmap created with AllocateMappedBitmap(), this will duplicate the
// handle to be passed to the display compositor, which can be in another
// process. The handle must be duplicated because we want to close the handle
// once it is mapped in this process. This will then close the original file
// handle in |memory| to free it up to the operating system. Some platforms
// have very limited namespaces for file handle counts and this avoids running
// out of them. Pass the same |size| as to AllocateMappedBitmap(), for
// debugging assistance.
VIZ_COMMON_EXPORT mojo::ScopedSharedBufferHandle DuplicateAndCloseMappedBitmap(
    base::SharedMemory* memory,
    const gfx::Size& size,
    ResourceFormat format);

// Similar to DuplicateAndCloseMappedBitmap(), but to be used in cases where the
// SharedMemory will have to be duplicated more than once. In that case the
// handle must be kept valid, so it can not be closed. Beware this will keep an
// open file handle on posix systems, which may contribute to surpassing handle
// limits.
VIZ_COMMON_EXPORT
mojo::ScopedSharedBufferHandle DuplicateWithoutClosingMappedBitmap(
    const base::SharedMemory* memory,
    const gfx::Size& size,
    ResourceFormat format);

}  // namespace bitmap_allocation

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_BITMAP_ALLOCATION_H_
