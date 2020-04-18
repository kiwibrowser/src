// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/resources/bitmap_allocation.h"

#include <stdint.h>

#include "base/debug/alias.h"
#include "base/memory/shared_memory.h"
#include "base/process/memory.h"
#include "build/build_config.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "components/viz/common/resources/resource_sizes.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "ui/gfx/geometry/size.h"

namespace viz {

namespace {
// Collect extra information for debugging bitmap creation failures.
void CollectMemoryUsageAndDie(const gfx::Size& size,
                              ResourceFormat format,
                              size_t alloc_size) {
#if defined(OS_WIN)
  DWORD last_error = GetLastError();
  base::debug::Alias(&last_error);
#endif

  int width = size.width();
  int height = size.height();

  base::debug::Alias(&width);
  base::debug::Alias(&height);
  base::debug::Alias(&format);

  base::TerminateBecauseOutOfMemory(alloc_size);
}
}  // namespace

namespace bitmap_allocation {

std::unique_ptr<base::SharedMemory> AllocateMappedBitmap(
    const gfx::Size& size,
    ResourceFormat format) {
  DCHECK(IsBitmapFormatSupported(format));
  size_t bytes = 0;
  if (!ResourceSizes::MaybeSizeInBytes(size, format, &bytes)) {
    DLOG(ERROR) << "AllocateMappedBitmap with size that overflows";
    CollectMemoryUsageAndDie(size, format, std::numeric_limits<int>::max());
  }

  auto mojo_buf = mojo::SharedBufferHandle::Create(bytes);
  if (!mojo_buf->is_valid()) {
    DLOG(ERROR) << "Browser failed to allocate shared memory";
    CollectMemoryUsageAndDie(size, format, bytes);
  }

  base::SharedMemoryHandle shared_buf;
  if (mojo::UnwrapSharedMemoryHandle(std::move(mojo_buf), &shared_buf, nullptr,
                                     nullptr) != MOJO_RESULT_OK) {
    DLOG(ERROR) << "Browser failed to allocate shared memory";
    CollectMemoryUsageAndDie(size, format, bytes);
  }

  auto memory = std::make_unique<base::SharedMemory>(shared_buf, false);
  if (!memory->Map(bytes)) {
    DLOG(ERROR) << "Browser failed to map shared memory";
    CollectMemoryUsageAndDie(size, format, bytes);
  }

  return memory;
}

mojo::ScopedSharedBufferHandle DuplicateAndCloseMappedBitmap(
    base::SharedMemory* memory,
    const gfx::Size& size,
    ResourceFormat format) {
  DCHECK(IsBitmapFormatSupported(format));
  base::SharedMemoryHandle dupe_handle =
      base::SharedMemory::DuplicateHandle(memory->handle());
  if (!base::SharedMemory::IsHandleValid(dupe_handle)) {
    DLOG(ERROR) << "Failed to duplicate shared memory handle for bitmap.";
    CollectMemoryUsageAndDie(size, format, memory->requested_size());
  }

  memory->Close();

  return mojo::WrapSharedMemoryHandle(
      dupe_handle, memory->mapped_size(),
      mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);
}

mojo::ScopedSharedBufferHandle DuplicateWithoutClosingMappedBitmap(
    const base::SharedMemory* memory,
    const gfx::Size& size,
    ResourceFormat format) {
  DCHECK(IsBitmapFormatSupported(format));
  base::SharedMemoryHandle dupe_handle =
      base::SharedMemory::DuplicateHandle(memory->handle());
  if (!base::SharedMemory::IsHandleValid(dupe_handle)) {
    DLOG(ERROR) << "Failed to duplicate shared memory handle for bitmap.";
    CollectMemoryUsageAndDie(size, format, memory->requested_size());
  }

  return mojo::WrapSharedMemoryHandle(
      dupe_handle, memory->mapped_size(),
      mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);
}

}  // namespace bitmap_allocation

}  // namespace viz
