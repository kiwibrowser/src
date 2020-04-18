// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/mock_gpu_memory_buffer_manager.h"

#include <memory>

#if defined(OS_CHROMEOS)
#include "media/capture/video/chromeos/stream_buffer_manager.h"
#endif

using ::testing::Return;

namespace media {
namespace unittest_internal {

namespace {

class FakeGpuMemoryBuffer : public gfx::GpuMemoryBuffer {
 public:
  FakeGpuMemoryBuffer(const gfx::Size& size, gfx::BufferFormat format)
      : size_(size), format_(format) {
    // We use only NV12 or R8 in unit tests.
    EXPECT_TRUE(format == gfx::BufferFormat::YUV_420_BIPLANAR ||
                format == gfx::BufferFormat::R_8);

    size_t y_plane_size = size_.width() * size_.height();
    size_t uv_plane_size = size_.width() * size_.height() / 2;
    data_ = std::vector<uint8_t>(y_plane_size + uv_plane_size);

    handle_.type = gfx::NATIVE_PIXMAP;
    // Set a dummy id since this is for testing only.
    handle_.id = gfx::GpuMemoryBufferId(0);

#if defined(OS_CHROMEOS)
    // Set a dummy fd since this is for testing only.
    handle_.native_pixmap_handle.fds.push_back(base::FileDescriptor(0, false));
    handle_.native_pixmap_handle.planes.push_back(
        gfx::NativePixmapPlane(size_.width(), 0, y_plane_size));
    handle_.native_pixmap_handle.planes.push_back(gfx::NativePixmapPlane(
        size_.width(), handle_.native_pixmap_handle.planes[0].size,
        uv_plane_size));

    // For faking a valid JPEG blob buffer.
    if (base::checked_cast<size_t>(size_.width()) >= sizeof(Camera3JpegBlob)) {
      Camera3JpegBlob* header = reinterpret_cast<Camera3JpegBlob*>(
          reinterpret_cast<uintptr_t>(data_.data()) + size_.width() -
          sizeof(Camera3JpegBlob));
      header->jpeg_blob_id = kCamera3JpegBlobId;
      header->jpeg_size = size_.width();
    }
#endif
  }

  ~FakeGpuMemoryBuffer() override = default;

  bool Map() override { return true; }

  void* memory(size_t plane) override {
    auto* data_ptr = data_.data();
    size_t y_plane_size = size_.width() * size_.height();
    switch (plane) {
      case 0:
        return reinterpret_cast<void*>(data_ptr);
      case 1:
        return reinterpret_cast<void*>(data_ptr + y_plane_size);
      default:
        NOTREACHED() << "Unsupported plane: " << plane;
        return nullptr;
    }
  }

  void Unmap() override {}

  gfx::Size GetSize() const override { return size_; }

  gfx::BufferFormat GetFormat() const override { return format_; }

  int stride(size_t plane) const override {
    switch (plane) {
      case 0:
        return size_.width();
      case 1:
        return size_.width();
      default:
        NOTREACHED() << "Unsupported plane: " << plane;
        return 0;
    }
  }

  void SetColorSpace(const gfx::ColorSpace& color_space) override {}

  gfx::GpuMemoryBufferId GetId() const override { return handle_.id; }

  gfx::GpuMemoryBufferHandle GetHandle() const override { return handle_; }

  ClientBuffer AsClientBuffer() override {
    NOTREACHED();
    return ClientBuffer();
  }

 private:
  gfx::Size size_;
  gfx::BufferFormat format_;
  std::vector<uint8_t> data_;
  gfx::GpuMemoryBufferHandle handle_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(FakeGpuMemoryBuffer);
};

}  // namespace

MockGpuMemoryBufferManager::MockGpuMemoryBufferManager() = default;

MockGpuMemoryBufferManager::~MockGpuMemoryBufferManager() = default;

// static
std::unique_ptr<gfx::GpuMemoryBuffer>
MockGpuMemoryBufferManager::CreateFakeGpuMemoryBuffer(
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    gpu::SurfaceHandle surface_handle) {
  return std::make_unique<FakeGpuMemoryBuffer>(size, format);
}

}  // namespace unittest_internal
}  // namespace media
