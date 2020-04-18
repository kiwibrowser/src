// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/gpu_memory_buffer.h"

#include "base/process/process_handle.h"
#include "ui/gfx/generic_shared_memory_id.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_ANDROID)
#include "base/android/android_hardware_buffer_compat.h"
#endif

namespace gfx {

GpuMemoryBufferHandle::GpuMemoryBufferHandle() : type(EMPTY_BUFFER), id(0) {}

GpuMemoryBufferHandle::GpuMemoryBufferHandle(
    const GpuMemoryBufferHandle& other) = default;

GpuMemoryBufferHandle::~GpuMemoryBufferHandle() = default;

void GpuMemoryBuffer::SetColorSpace(const gfx::ColorSpace& color_space) {}

GpuMemoryBufferHandle CloneHandleForIPC(
    const GpuMemoryBufferHandle& source_handle) {
  switch (source_handle.type) {
    case gfx::EMPTY_BUFFER:
      NOTREACHED();
      return source_handle;
    case gfx::SHARED_MEMORY_BUFFER: {
      gfx::GpuMemoryBufferHandle handle;
      handle.type = gfx::SHARED_MEMORY_BUFFER;
      handle.id = source_handle.id;
      handle.handle = base::SharedMemory::DuplicateHandle(source_handle.handle);
      handle.offset = source_handle.offset;
      handle.stride = source_handle.stride;
      return handle;
    }
    case gfx::NATIVE_PIXMAP: {
      gfx::GpuMemoryBufferHandle handle;
      handle.type = gfx::NATIVE_PIXMAP;
      handle.id = source_handle.id;
#if defined(OS_LINUX)
      handle.native_pixmap_handle =
          gfx::CloneHandleForIPC(source_handle.native_pixmap_handle);
#endif
      return handle;
    }
    case gfx::ANDROID_HARDWARE_BUFFER: {
      gfx::GpuMemoryBufferHandle handle;
      handle.type = gfx::ANDROID_HARDWARE_BUFFER;
      handle.id = source_handle.id;
#if defined(OS_ANDROID)
      base::AndroidHardwareBufferCompat::GetInstance().Acquire(
          source_handle.android_hardware_buffer);
      handle.android_hardware_buffer = source_handle.android_hardware_buffer;
#endif
      return handle;
    }
    case gfx::IO_SURFACE_BUFFER:
      return source_handle;
    case gfx::DXGI_SHARED_HANDLE:
      gfx::GpuMemoryBufferHandle handle;
      handle.type = gfx::DXGI_SHARED_HANDLE;
      handle.id = source_handle.id;
#if defined(OS_WIN)
      base::ProcessHandle process = ::GetCurrentProcess();
      HANDLE duplicated_handle;
      BOOL result = ::DuplicateHandle(
          process, source_handle.dxgi_handle.GetHandle(), process,
          &duplicated_handle, 0, FALSE, DUPLICATE_SAME_ACCESS);
      if (!result)
        DPLOG(ERROR) << "Failed to duplicate DXGI resource handle.";
      handle.dxgi_handle = IPC::PlatformFileForTransit(duplicated_handle);
#endif
      return handle;
  }
  return gfx::GpuMemoryBufferHandle();
}

base::trace_event::MemoryAllocatorDumpGuid GpuMemoryBuffer::GetGUIDForTracing(
    uint64_t tracing_process_id) const {
  return gfx::GetGenericSharedGpuMemoryGUIDForTracing(tracing_process_id,
                                                      GetId());
}

}  // namespace gfx
