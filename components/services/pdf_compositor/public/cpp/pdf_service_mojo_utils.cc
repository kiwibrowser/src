// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/pdf_compositor/public/cpp/pdf_service_mojo_utils.h"

#include <utility>

#include "base/memory/shared_memory.h"
#include "base/memory/writable_shared_memory_region.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace printing {

base::MappedReadOnlyRegion CreateReadOnlySharedMemoryRegion(size_t size) {
  mojo::ScopedSharedBufferHandle handle =
      mojo::SharedBufferHandle::Create(size);
  base::WritableSharedMemoryRegion writable_region =
      UnwrapWritableSharedMemoryRegion(std::move(handle));
  base::WritableSharedMemoryMapping mapping = writable_region.Map();
  if (!mapping.IsValid())
    return {};

  base::ReadOnlySharedMemoryRegion readonly_region =
      base::WritableSharedMemoryRegion::ConvertToReadOnly(
          std::move(writable_region));
  return {std::move(readonly_region), std::move(mapping)};
}

std::unique_ptr<base::SharedMemory> GetShmFromMojoHandle(
    mojo::ScopedSharedBufferHandle handle) {
  base::SharedMemoryHandle memory_handle;
  size_t memory_size = 0;
  mojo::UnwrappedSharedMemoryHandleProtection protection;

  const MojoResult result = mojo::UnwrapSharedMemoryHandle(
      std::move(handle), &memory_handle, &memory_size, &protection);
  if (result != MOJO_RESULT_OK)
    return nullptr;

  DCHECK_GT(memory_size, 0u);
  const bool read_only =
      protection == mojo::UnwrappedSharedMemoryHandleProtection::kReadOnly;
  std::unique_ptr<base::SharedMemory> shm =
      std::make_unique<base::SharedMemory>(memory_handle, read_only);
  if (!shm->Map(memory_size)) {
    DLOG(ERROR) << "Map shared memory failed.";
    return nullptr;
  }
  return shm;
}

}  // namespace printing
