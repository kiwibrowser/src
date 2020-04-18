// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/common/gpu_memory_buffer_impl_shared_memory.h"

#include <stdint.h>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/format_macros.h"
#include "base/memory/ptr_util.h"
#include "base/numerics/safe_math.h"
#include "base/process/memory.h"
#include "base/strings/stringprintf.h"
#include "ui/gfx/buffer_format_util.h"
#include "ui/gl/gl_bindings.h"

namespace gpu {

GpuMemoryBufferImplSharedMemory::GpuMemoryBufferImplSharedMemory(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    const DestructionCallback& callback,
    std::unique_ptr<base::SharedMemory> shared_memory,
    size_t offset,
    int stride)
    : GpuMemoryBufferImpl(id, size, format, callback),
      shared_memory_(std::move(shared_memory)),
      offset_(offset),
      stride_(stride) {
  DCHECK(IsUsageSupported(usage));
  DCHECK(IsSizeValidForFormat(size, format));
}

GpuMemoryBufferImplSharedMemory::~GpuMemoryBufferImplSharedMemory() = default;

// static
std::unique_ptr<GpuMemoryBufferImplSharedMemory>
GpuMemoryBufferImplSharedMemory::Create(gfx::GpuMemoryBufferId id,
                                        const gfx::Size& size,
                                        gfx::BufferFormat format,
                                        gfx::BufferUsage usage,
                                        const DestructionCallback& callback) {
  if (!IsUsageSupported(usage))
    return nullptr;
  size_t buffer_size = 0u;
  if (!gfx::BufferSizeForBufferFormatChecked(size, format, &buffer_size))
    return nullptr;

  std::unique_ptr<base::SharedMemory> shared_memory(new base::SharedMemory());
  if (!shared_memory->CreateAndMapAnonymous(buffer_size))
    return nullptr;

  return base::WrapUnique(new GpuMemoryBufferImplSharedMemory(
      id, size, format, usage, callback, std::move(shared_memory), 0,
      gfx::RowSizeForBufferFormat(size.width(), format, 0)));
}

// static
gfx::GpuMemoryBufferHandle
GpuMemoryBufferImplSharedMemory::CreateGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage) {
  if (!IsUsageSupported(usage))
    return gfx::GpuMemoryBufferHandle();
  size_t buffer_size = 0u;
  if (!gfx::BufferSizeForBufferFormatChecked(size, format, &buffer_size))
    return gfx::GpuMemoryBufferHandle();

  base::SharedMemory shared_memory;
  if (!shared_memory.CreateAnonymous(buffer_size))
    return gfx::GpuMemoryBufferHandle();

  gfx::GpuMemoryBufferHandle handle;
  handle.type = gfx::SHARED_MEMORY_BUFFER;
  handle.id = id;
  handle.offset = 0;
  handle.stride = static_cast<int32_t>(
      gfx::RowSizeForBufferFormat(size.width(), format, 0));
  handle.handle = shared_memory.TakeHandle();
  return handle;
}

// static
std::unique_ptr<GpuMemoryBufferImplSharedMemory>
GpuMemoryBufferImplSharedMemory::CreateFromHandle(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    const DestructionCallback& callback) {
  DCHECK(base::SharedMemory::IsHandleValid(handle.handle));

  return base::WrapUnique(new GpuMemoryBufferImplSharedMemory(
      handle.id, size, format, usage, callback,
      std::make_unique<base::SharedMemory>(handle.handle, false), handle.offset,
      handle.stride));
}

// static
bool GpuMemoryBufferImplSharedMemory::IsUsageSupported(gfx::BufferUsage usage) {
  switch (usage) {
    case gfx::BufferUsage::GPU_READ:
    case gfx::BufferUsage::GPU_READ_CPU_READ_WRITE:
    case gfx::BufferUsage::GPU_READ_CPU_READ_WRITE_PERSISTENT:
    case gfx::BufferUsage::SCANOUT_CPU_READ_WRITE:
      return true;
    case gfx::BufferUsage::SCANOUT:
    case gfx::BufferUsage::SCANOUT_CAMERA_READ_WRITE:
    case gfx::BufferUsage::CAMERA_AND_CPU_READ_WRITE:
    case gfx::BufferUsage::SCANOUT_VDA_WRITE:
      return false;
  }
  NOTREACHED();
  return false;
}

// static
bool GpuMemoryBufferImplSharedMemory::IsConfigurationSupported(
    gfx::BufferFormat format,
    gfx::BufferUsage usage) {
  return IsUsageSupported(usage);
}

// static
bool GpuMemoryBufferImplSharedMemory::IsSizeValidForFormat(
    const gfx::Size& size,
    gfx::BufferFormat format) {
  switch (format) {
    case gfx::BufferFormat::ATC:
    case gfx::BufferFormat::ATCIA:
    case gfx::BufferFormat::DXT1:
    case gfx::BufferFormat::DXT5:
    case gfx::BufferFormat::ETC1:
      // Compressed images must have a width and height that's evenly divisible
      // by the block size.
      return size.width() % 4 == 0 && size.height() % 4 == 0;
    case gfx::BufferFormat::R_8:
    case gfx::BufferFormat::R_16:
    case gfx::BufferFormat::RG_88:
    case gfx::BufferFormat::BGR_565:
    case gfx::BufferFormat::RGBA_4444:
    case gfx::BufferFormat::RGBA_8888:
    case gfx::BufferFormat::RGBX_8888:
    case gfx::BufferFormat::BGRA_8888:
    case gfx::BufferFormat::BGRX_8888:
    case gfx::BufferFormat::BGRX_1010102:
    case gfx::BufferFormat::RGBX_1010102:
    case gfx::BufferFormat::RGBA_F16:
      return true;
    case gfx::BufferFormat::YVU_420:
    case gfx::BufferFormat::YUV_420_BIPLANAR: {
      size_t num_planes = gfx::NumberOfPlanesForBufferFormat(format);
      for (size_t i = 0; i < num_planes; ++i) {
        size_t factor = gfx::SubsamplingFactorForBufferFormat(format, i);
        if (size.width() % factor || size.height() % factor)
          return false;
      }
      return true;
    }
    case gfx::BufferFormat::UYVY_422:
      return size.width() % 2 == 0;
  }

  NOTREACHED();
  return false;
}

// static
base::Closure GpuMemoryBufferImplSharedMemory::AllocateForTesting(
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    gfx::GpuMemoryBufferHandle* handle) {
  *handle = CreateGpuMemoryBuffer(handle->id, size, format, usage);
  return base::DoNothing();
}

bool GpuMemoryBufferImplSharedMemory::Map() {
  DCHECK(!mapped_);

  // Map the buffer first time Map() is called then keep it mapped for the
  // lifetime of the buffer. This avoids mapping the buffer unless necessary.
  if (!shared_memory_->memory()) {
    DCHECK_EQ(static_cast<size_t>(stride_),
              gfx::RowSizeForBufferFormat(size_.width(), format_, 0));
    size_t buffer_size = gfx::BufferSizeForBufferFormat(size_, format_);
    // Note: offset_ != 0 is not common use-case. To keep it simple we
    // map offset + buffer_size here but this can be avoided using MapAt().
    size_t map_size = offset_ + buffer_size;
    if (!shared_memory_->Map(map_size))
      base::TerminateBecauseOutOfMemory(map_size);
  }
  mapped_ = true;
  return true;
}

void* GpuMemoryBufferImplSharedMemory::memory(size_t plane) {
  DCHECK(mapped_);
  DCHECK_LT(plane, gfx::NumberOfPlanesForBufferFormat(format_));
  return reinterpret_cast<uint8_t*>(shared_memory_->memory()) + offset_ +
         gfx::BufferOffsetForBufferFormat(size_, format_, plane);
}

void GpuMemoryBufferImplSharedMemory::Unmap() {
  DCHECK(mapped_);
  mapped_ = false;
}

int GpuMemoryBufferImplSharedMemory::stride(size_t plane) const {
  DCHECK_LT(plane, gfx::NumberOfPlanesForBufferFormat(format_));
  return gfx::RowSizeForBufferFormat(size_.width(), format_, plane);
}

gfx::GpuMemoryBufferHandle GpuMemoryBufferImplSharedMemory::GetHandle() const {
  gfx::GpuMemoryBufferHandle handle;
  handle.type = gfx::SHARED_MEMORY_BUFFER;
  handle.id = id_;
  handle.offset = offset_;
  handle.stride = stride_;
  handle.handle = shared_memory_->handle();
  return handle;
}

base::trace_event::MemoryAllocatorDumpGuid
GpuMemoryBufferImplSharedMemory::GetGUIDForTracing(
    uint64_t tracing_process_id) const {
  return base::trace_event::MemoryAllocatorDumpGuid(base::StringPrintf(
      "shared_memory_gpu/%" PRIx64 "/%d", tracing_process_id, id_.id));
}

}  // namespace gpu
