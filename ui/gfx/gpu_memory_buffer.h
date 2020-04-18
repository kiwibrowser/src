// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_GPU_MEMORY_BUFFER_H_
#define UI_GFX_GPU_MEMORY_BUFFER_H_

#include <stddef.h>
#include <stdint.h>

#include "base/memory/shared_memory.h"
#include "base/trace_event/memory_allocator_dump_guid.h"
#include "build/build_config.h"
#include "ui/gfx/buffer_types.h"
#include "ui/gfx/generic_shared_memory_id.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/gfx_export.h"

#if defined(OS_LINUX)
#include "ui/gfx/native_pixmap_handle.h"
#elif defined(OS_MACOSX) && !defined(OS_IOS)
#include "ui/gfx/mac/io_surface.h"
#elif defined(OS_WIN)
#include "ipc/ipc_platform_file.h"  // nogncheck
#endif

extern "C" typedef struct _ClientBuffer* ClientBuffer;

#if defined(OS_ANDROID)
extern "C" typedef struct AHardwareBuffer AHardwareBuffer;
#endif

namespace gfx {

class ColorSpace;

enum GpuMemoryBufferType {
  EMPTY_BUFFER,
  SHARED_MEMORY_BUFFER,
  IO_SURFACE_BUFFER,
  NATIVE_PIXMAP,
  DXGI_SHARED_HANDLE,
  ANDROID_HARDWARE_BUFFER,
  GPU_MEMORY_BUFFER_TYPE_LAST = ANDROID_HARDWARE_BUFFER
};

using GpuMemoryBufferId = GenericSharedMemoryId;

struct GFX_EXPORT GpuMemoryBufferHandle {
  GpuMemoryBufferHandle();
  GpuMemoryBufferHandle(const GpuMemoryBufferHandle& other);
  ~GpuMemoryBufferHandle();
  bool is_null() const { return type == EMPTY_BUFFER; }
  GpuMemoryBufferType type;
  GpuMemoryBufferId id;
  base::SharedMemoryHandle handle;
  uint32_t offset;
  int32_t stride;
#if defined(OS_LINUX)
  NativePixmapHandle native_pixmap_handle;
#elif defined(OS_MACOSX) && !defined(OS_IOS)
  ScopedRefCountedIOSurfaceMachPort mach_port;
#elif defined(OS_WIN)
  IPC::PlatformFileForTransit dxgi_handle;
#elif defined(OS_ANDROID)
  AHardwareBuffer* android_hardware_buffer = nullptr;
#endif
};

// This interface typically correspond to a type of shared memory that is also
// shared with the GPU. A GPU memory buffer can be written to directly by
// regular CPU code, but can also be read by the GPU.
class GFX_EXPORT GpuMemoryBuffer {
 public:
  virtual ~GpuMemoryBuffer() {}

  // Maps each plane of the buffer into the client's address space so it can be
  // written to by the CPU. This call may block, for instance if the GPU needs
  // to finish accessing the buffer or if CPU caches need to be synchronized.
  // Returns false on failure.
  virtual bool Map() = 0;

  // Returns a pointer to the memory address of a plane. Buffer must have been
  // successfully mapped using a call to Map() before calling this function.
  virtual void* memory(size_t plane) = 0;

  // Unmaps the buffer. It's illegal to use any pointer returned by memory()
  // after this has been called.
  virtual void Unmap() = 0;

  // Returns the size for the buffer.
  virtual Size GetSize() const = 0;

  // Returns the format for the buffer.
  virtual BufferFormat GetFormat() const = 0;

  // Fills the stride in bytes for each plane of the buffer. The stride of
  // plane K is stored at index K-1 of the |stride| array.
  virtual int stride(size_t plane) const = 0;

  // Set the color space in which this buffer should be interpreted when used
  // as an overlay. Note that this will not impact texturing from the buffer.
  virtual void SetColorSpace(const gfx::ColorSpace& color_space);

  // Returns a unique identifier associated with buffer.
  virtual GpuMemoryBufferId GetId() const = 0;

  // Returns a platform specific handle for this buffer.
  virtual GpuMemoryBufferHandle GetHandle() const = 0;

  // Type-checking downcast routine.
  virtual ClientBuffer AsClientBuffer() = 0;

  // Returns the GUID for tracing.
  virtual base::trace_event::MemoryAllocatorDumpGuid GetGUIDForTracing(
      uint64_t tracing_process_id) const;
};

// Returns an instance of |handle| which can be sent over IPC. This duplicates
// the file-handles as appropriate, so that the IPC code take ownership of them,
// without invalidating |handle| itself.
GFX_EXPORT GpuMemoryBufferHandle
CloneHandleForIPC(const GpuMemoryBufferHandle& handle);

}  // namespace gfx

#endif  // UI_GFX_GPU_MEMORY_BUFFER_H_
