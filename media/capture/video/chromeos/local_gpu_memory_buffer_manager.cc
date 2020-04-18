// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/chromeos/local_gpu_memory_buffer_manager.h"

#include <drm_fourcc.h>
#include <xf86drm.h>
#include <memory>

namespace media {

namespace {

const int32_t kDrmNumNodes = 64;
const int32_t kMinNodeNumber = 128;

gbm_device* CreateGbmDevice() {
  int fd;
  int32_t min_node = kMinNodeNumber;
  int32_t max_node = kMinNodeNumber + kDrmNumNodes;
  struct gbm_device* gbm = nullptr;

  for (int i = min_node; i < max_node; i++) {
    fd = drmOpenRender(i);
    if (fd < 0) {
      continue;
    }

    drmVersionPtr version = drmGetVersion(fd);
    if (!strcmp("vgem", version->name)) {
      drmFreeVersion(version);
      close(fd);
      continue;
    }

    gbm = gbm_create_device(fd);
    if (!gbm) {
      drmFreeVersion(version);
      close(fd);
      continue;
    }

    VLOG(1) << "Opened gbm device on render node " << version->name;
    drmFreeVersion(version);
    return gbm;
  }

  return nullptr;
}

uint32_t GetDrmFormat(gfx::BufferFormat gfx_format) {
  switch (gfx_format) {
    case gfx::BufferFormat::R_8:
      return DRM_FORMAT_R8;
    case gfx::BufferFormat::YUV_420_BIPLANAR:
      return DRM_FORMAT_NV12;
    // Add more formats when needed.
    default:
      return 0;
  }
}

class GpuMemoryBufferImplGbm : public gfx::GpuMemoryBuffer {
 public:
  GpuMemoryBufferImplGbm(gfx::BufferFormat format, gbm_bo* buffer_object)
      : format_(format), buffer_object_(buffer_object), mapped_(false) {
    handle_.type = gfx::NATIVE_PIXMAP;
    // Set a dummy id since this is for testing only.
    handle_.id = gfx::GpuMemoryBufferId(0);
    handle_.native_pixmap_handle.fds.push_back(
        base::FileDescriptor(gbm_bo_get_fd(buffer_object), false));
    for (size_t i = 0; i < gbm_bo_get_num_planes(buffer_object); ++i) {
      handle_.native_pixmap_handle.planes.push_back(
          gfx::NativePixmapPlane(gbm_bo_get_plane_stride(buffer_object, i),
                                 gbm_bo_get_plane_offset(buffer_object, i),
                                 gbm_bo_get_plane_size(buffer_object, i)));
    }
  }

  ~GpuMemoryBufferImplGbm() override {
    if (mapped_) {
      Unmap();
    }
    close(gbm_bo_get_fd(buffer_object_));
    gbm_bo_destroy(buffer_object_);
  }

  bool Map() override {
    if (mapped_) {
      return true;
    }
    size_t num_planes = gbm_bo_get_num_planes(buffer_object_);
    uint32_t stride;
    mapped_planes_.resize(num_planes);
    for (size_t i = 0; i < num_planes; ++i) {
      void* mapped_data;
      void* addr =
          gbm_bo_map(buffer_object_, 0, 0, gbm_bo_get_width(buffer_object_),
                     gbm_bo_get_height(buffer_object_),
                     GBM_BO_TRANSFER_READ_WRITE, &stride, &mapped_data, i);
      if (!addr) {
        LOG(ERROR) << "Failed to map GpuMemoryBufferImplGbm plane " << i;
        Unmap();
        return false;
      }
      mapped_planes_[i].addr = addr;
      mapped_planes_[i].mapped_data = mapped_data;
    }
    mapped_ = true;
    return true;
  };

  void* memory(size_t plane) override {
    if (!mapped_) {
      LOG(ERROR) << "Buffer is not mapped";
      return nullptr;
    }
    if (plane > mapped_planes_.size()) {
      LOG(ERROR) << "Invalid plane: " << plane;
      return nullptr;
    }
    return mapped_planes_[plane].addr;
  }

  void Unmap() override {
    for (size_t i = 0; i < mapped_planes_.size(); ++i) {
      if (mapped_planes_[i].addr) {
        gbm_bo_unmap(buffer_object_, mapped_planes_[i].mapped_data);
        mapped_planes_[i].addr = nullptr;
        mapped_planes_[i].mapped_data = nullptr;
      }
    }
    mapped_planes_.clear();
    mapped_ = false;
  }

  gfx::Size GetSize() const override {
    return gfx::Size(gbm_bo_get_width(buffer_object_),
                     gbm_bo_get_height(buffer_object_));
  }

  gfx::BufferFormat GetFormat() const override { return format_; }

  int stride(size_t plane) const override {
    return gbm_bo_get_plane_stride(buffer_object_, plane);
  }

  void SetColorSpace(const gfx::ColorSpace& color_space) override {}

  gfx::GpuMemoryBufferId GetId() const override { return handle_.id; }

  gfx::GpuMemoryBufferHandle GetHandle() const override { return handle_; }

  ClientBuffer AsClientBuffer() override {
    return reinterpret_cast<ClientBuffer>(this);
  }

 private:
  struct MappedPlane {
    void* addr;
    void* mapped_data;
  };

  gfx::BufferFormat format_;
  gbm_bo* buffer_object_;
  gfx::GpuMemoryBufferHandle handle_;
  bool mapped_;
  std::vector<MappedPlane> mapped_planes_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(GpuMemoryBufferImplGbm);
};

}  // namespace

LocalGpuMemoryBufferManager::LocalGpuMemoryBufferManager()
    : gbm_device_(CreateGbmDevice()) {}

LocalGpuMemoryBufferManager::~LocalGpuMemoryBufferManager() {
  if (gbm_device_) {
    close(gbm_device_get_fd(gbm_device_));
    gbm_device_destroy(gbm_device_);
  }
}

std::unique_ptr<gfx::GpuMemoryBuffer>
LocalGpuMemoryBufferManager::CreateGpuMemoryBuffer(
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    gpu::SurfaceHandle surface_handle) {
  if (usage != gfx::BufferUsage::SCANOUT_CAMERA_READ_WRITE &&
      usage != gfx::BufferUsage::CAMERA_AND_CPU_READ_WRITE) {
    LOG(ERROR) << "Unsupported gfx::BufferUsage" << static_cast<int>(usage);
    return std::unique_ptr<gfx::GpuMemoryBuffer>();
  }
  if (!gbm_device_) {
    LOG(ERROR) << "Invalid GBM device";
    return std::unique_ptr<gfx::GpuMemoryBuffer>();
  }

  uint32_t drm_format = GetDrmFormat(format);
  uint32_t camera_gbm_usage =
      GBM_BO_USE_LINEAR | GBM_BO_USE_CAMERA_READ | GBM_BO_USE_CAMERA_WRITE;
  if (!drm_format) {
    LOG(ERROR) << "Unable to convert gfx::BufferFormat "
               << static_cast<int>(format) << " to DRM format";
    return std::unique_ptr<gfx::GpuMemoryBuffer>();
  }

  if (!gbm_device_is_format_supported(gbm_device_, drm_format,
                                      camera_gbm_usage)) {
    return std::unique_ptr<gfx::GpuMemoryBuffer>();
  }

  gbm_bo* buffer_object = gbm_bo_create(
      gbm_device_, size.width(), size.height(), drm_format, camera_gbm_usage);
  if (!buffer_object) {
    LOG(ERROR) << "Failed to create GBM buffer object";
    return std::unique_ptr<gfx::GpuMemoryBuffer>();
  }

  return std::make_unique<GpuMemoryBufferImplGbm>(format, buffer_object);
}

void LocalGpuMemoryBufferManager::SetDestructionSyncToken(
    gfx::GpuMemoryBuffer* buffer,
    const gpu::SyncToken& sync_token) {}

}  // namespace media
