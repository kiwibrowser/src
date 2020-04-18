// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/mojo/buffer_types_struct_traits.h"

#include "build/build_config.h"
#include "mojo/public/cpp/system/platform_handle.h"

#if defined(OS_ANDROID)
#include "base/android/scoped_hardware_buffer_handle.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/scope_to_message_pipe.h"
#endif

namespace mojo {

// static
bool StructTraits<gfx::mojom::BufferUsageAndFormatDataView,
                  gfx::BufferUsageAndFormat>::
    Read(gfx::mojom::BufferUsageAndFormatDataView data,
         gfx::BufferUsageAndFormat* out) {
  return data.ReadUsage(&out->usage) && data.ReadFormat(&out->format);
}

std::vector<mojo::ScopedHandle>
StructTraits<gfx::mojom::NativePixmapHandleDataView, gfx::NativePixmapHandle>::
    fds(const gfx::NativePixmapHandle& pixmap_handle) {
  std::vector<mojo::ScopedHandle> handles;
#if defined(OS_LINUX)
  for (const base::FileDescriptor& fd : pixmap_handle.fds)
    handles.emplace_back(mojo::WrapPlatformFile(fd.fd));
#endif  // defined(OS_LINUX)
  return handles;
}

bool StructTraits<
    gfx::mojom::NativePixmapHandleDataView,
    gfx::NativePixmapHandle>::Read(gfx::mojom::NativePixmapHandleDataView data,
                                   gfx::NativePixmapHandle* out) {
#if defined(OS_LINUX)
  mojo::ArrayDataView<mojo::ScopedHandle> handles_data_view;
  data.GetFdsDataView(&handles_data_view);
  for (size_t i = 0; i < handles_data_view.size(); ++i) {
    mojo::ScopedHandle handle = handles_data_view.Take(i);
    base::PlatformFile platform_file;
    if (mojo::UnwrapPlatformFile(std::move(handle), &platform_file) !=
        MOJO_RESULT_OK)
      return false;
    constexpr bool auto_close = true;
    out->fds.push_back(base::FileDescriptor(platform_file, auto_close));
  }
  return data.ReadPlanes(&out->planes);
#else
  return false;
#endif
}

mojo::ScopedSharedBufferHandle
StructTraits<gfx::mojom::GpuMemoryBufferHandleDataView,
             gfx::GpuMemoryBufferHandle>::
    shared_memory_handle(const gfx::GpuMemoryBufferHandle& handle) {
  if (handle.type != gfx::SHARED_MEMORY_BUFFER)
    return mojo::ScopedSharedBufferHandle();
  return mojo::WrapSharedMemoryHandle(
      handle.handle, handle.handle.GetSize(),
      mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);
}

const gfx::NativePixmapHandle&
StructTraits<gfx::mojom::GpuMemoryBufferHandleDataView,
             gfx::GpuMemoryBufferHandle>::
    native_pixmap_handle(const gfx::GpuMemoryBufferHandle& handle) {
#if defined(OS_LINUX)
  return handle.native_pixmap_handle;
#else
  static gfx::NativePixmapHandle pixmap_handle;
  return pixmap_handle;
#endif
}

mojo::ScopedHandle StructTraits<gfx::mojom::GpuMemoryBufferHandleDataView,
                                gfx::GpuMemoryBufferHandle>::
    mach_port(const gfx::GpuMemoryBufferHandle& handle) {
#if defined(OS_MACOSX) && !defined(OS_IOS)
  if (handle.type != gfx::IO_SURFACE_BUFFER)
    return mojo::ScopedHandle();
  return mojo::WrapMachPort(handle.mach_port.get());
#else
  return mojo::ScopedHandle();
#endif
}

#if defined(OS_WIN)
// static
mojo::ScopedHandle StructTraits<gfx::mojom::GpuMemoryBufferHandleDataView,
                                gfx::GpuMemoryBufferHandle>::
    dxgi_handle(const gfx::GpuMemoryBufferHandle& handle) {
  if (handle.type != gfx::DXGI_SHARED_HANDLE)
    return mojo::ScopedHandle();
  DCHECK(handle.dxgi_handle.IsValid());
  return mojo::WrapPlatformFile(handle.dxgi_handle.GetHandle());
}
#endif

#if defined(OS_ANDROID)
// static
gfx::mojom::AHardwareBufferHandlePtr
StructTraits<gfx::mojom::GpuMemoryBufferHandleDataView,
             gfx::GpuMemoryBufferHandle>::
    android_hardware_buffer_handle(const gfx::GpuMemoryBufferHandle& handle) {
  if (handle.type != gfx::ANDROID_HARDWARE_BUFFER)
    return nullptr;

  // Assume ownership of the input AHardwareBuffer.
  auto scoped_handle = base::android::ScopedHardwareBufferHandle::Adopt(
      handle.android_hardware_buffer);

  // We must keep a ref to the AHardwareBuffer alive until the receiver has
  // acquired its own reference. We do this by sending a message pipe handle
  // along with the buffer. When the receiver deserializes (or even if they
  // die without ever reading the message) their end of the pipe will be
  // closed. We will eventually detect this and release the AHB reference.
  mojo::MessagePipe tracking_pipe;
  auto wrapped_handle = gfx::mojom::AHardwareBufferHandle::New(
      mojo::WrapPlatformFile(
          scoped_handle.SerializeAsFileDescriptor().release()),
      std::move(tracking_pipe.handle0));

  // Pass ownership of the input handle to our tracking pipe to keep the AHB
  // alive until it's deserialized.
  mojo::ScopeToMessagePipe(std::move(scoped_handle),
                           std::move(tracking_pipe.handle1));
  return wrapped_handle;
}
#endif

bool StructTraits<gfx::mojom::GpuMemoryBufferHandleDataView,
                  gfx::GpuMemoryBufferHandle>::
    Read(gfx::mojom::GpuMemoryBufferHandleDataView data,
         gfx::GpuMemoryBufferHandle* out) {
  if (!data.ReadType(&out->type) || !data.ReadId(&out->id))
    return false;

  if (out->type == gfx::SHARED_MEMORY_BUFFER) {
    mojo::ScopedSharedBufferHandle handle = data.TakeSharedMemoryHandle();
    if (handle.is_valid()) {
      MojoResult unwrap_result = mojo::UnwrapSharedMemoryHandle(
          std::move(handle), &out->handle, nullptr, nullptr);
      if (unwrap_result != MOJO_RESULT_OK)
        return false;
    }

    out->offset = data.offset();
    out->stride = data.stride();
  }
#if defined(OS_LINUX)
  if (out->type == gfx::NATIVE_PIXMAP &&
      !data.ReadNativePixmapHandle(&out->native_pixmap_handle))
    return false;
#endif
#if defined(OS_MACOSX) && !defined(OS_IOS)
  if (out->type == gfx::IO_SURFACE_BUFFER) {
    mach_port_t mach_port;
    MojoResult unwrap_result =
        mojo::UnwrapMachPort(data.TakeMachPort(), &mach_port);
    if (unwrap_result != MOJO_RESULT_OK)
      return false;
    out->mach_port.reset(mach_port);
  }
#endif
#if defined(OS_WIN)
  if (out->type == gfx::DXGI_SHARED_HANDLE) {
    HANDLE handle;
    MojoResult unwrap_result =
        mojo::UnwrapPlatformFile(data.TakeDxgiHandle(), &handle);
    if (unwrap_result != MOJO_RESULT_OK)
      return false;
    out->dxgi_handle = IPC::PlatformFileForTransit(handle);
    out->offset = data.offset();
    out->stride = data.stride();
  }
#endif
#if defined(OS_ANDROID)
  if (out->type == gfx::ANDROID_HARDWARE_BUFFER) {
    gfx::mojom::AHardwareBufferHandlePtr buffer_handle;
    if (!data.ReadAndroidHardwareBufferHandle(&buffer_handle) || !buffer_handle)
      return false;

    base::PlatformFile fd;
    MojoResult unwrap_result =
        mojo::UnwrapPlatformFile(std::move(buffer_handle->buffer_handle), &fd);
    base::ScopedFD scoped_fd(fd);
    if (unwrap_result != MOJO_RESULT_OK || !scoped_fd.is_valid())
      return false;
    out->android_hardware_buffer =
        base::android::ScopedHardwareBufferHandle ::
            DeserializeFromFileDescriptor(std::move(scoped_fd))
                .Take();
    out->offset = data.offset();
    out->stride = data.stride();
  }
#endif
  return true;
}

}  // namespace mojo
