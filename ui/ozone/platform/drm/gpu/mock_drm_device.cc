// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/mock_drm_device.h"

#include <xf86drm.h>

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/ozone/platform/drm/gpu/mock_hardware_display_plane_manager.h"

namespace ui {

namespace {

template <class Object>
Object* DrmAllocator() {
  return static_cast<Object*>(drmMalloc(sizeof(Object)));
}

}  // namespace

MockDrmDevice::MockDrmDevice()
    : DrmDevice(base::FilePath(), base::File(), true /* is_primary_device */),
      get_crtc_call_count_(0),
      set_crtc_call_count_(0),
      restore_crtc_call_count_(0),
      add_framebuffer_call_count_(0),
      remove_framebuffer_call_count_(0),
      page_flip_call_count_(0),
      overlay_flip_call_count_(0),
      overlay_clear_call_count_(0),
      allocate_buffer_count_(0),
      set_crtc_expectation_(true),
      add_framebuffer_expectation_(true),
      page_flip_expectation_(true),
      create_dumb_buffer_expectation_(true),
      use_sync_flips_(false),
      current_framebuffer_(0) {
  plane_manager_.reset(new HardwareDisplayPlaneManagerLegacy());
}

MockDrmDevice::MockDrmDevice(bool use_sync_flips,
                             std::vector<uint32_t> crtcs,
                             size_t planes_per_crtc)
    : DrmDevice(base::FilePath(), base::File(), true /* is_primary_device */),
      get_crtc_call_count_(0),
      set_crtc_call_count_(0),
      restore_crtc_call_count_(0),
      add_framebuffer_call_count_(0),
      remove_framebuffer_call_count_(0),
      page_flip_call_count_(0),
      overlay_flip_call_count_(0),
      overlay_clear_call_count_(0),
      allocate_buffer_count_(0),
      set_crtc_expectation_(true),
      add_framebuffer_expectation_(true),
      page_flip_expectation_(true),
      create_dumb_buffer_expectation_(true),
      use_sync_flips_(use_sync_flips),
      current_framebuffer_(0) {
  plane_manager_.reset(
      new MockHardwareDisplayPlaneManager(this, crtcs, planes_per_crtc));
}

MockDrmDevice::~MockDrmDevice() {}

ScopedDrmResourcesPtr MockDrmDevice::GetResources() {
  return ScopedDrmResourcesPtr(DrmAllocator<drmModeRes>());
}

ScopedDrmObjectPropertyPtr MockDrmDevice::GetObjectProperties(
    uint32_t object_id,
    uint32_t object_type) {
  return ScopedDrmObjectPropertyPtr(DrmAllocator<drmModeObjectProperties>());
}

ScopedDrmCrtcPtr MockDrmDevice::GetCrtc(uint32_t crtc_id) {
  get_crtc_call_count_++;
  return ScopedDrmCrtcPtr(DrmAllocator<drmModeCrtc>());
}

bool MockDrmDevice::SetCrtc(uint32_t crtc_id,
                            uint32_t framebuffer,
                            std::vector<uint32_t> connectors,
                            drmModeModeInfo* mode) {
  current_framebuffer_ = framebuffer;
  set_crtc_call_count_++;
  return set_crtc_expectation_;
}

bool MockDrmDevice::SetCrtc(drmModeCrtc* crtc,
                            std::vector<uint32_t> connectors) {
  restore_crtc_call_count_++;
  return true;
}

bool MockDrmDevice::DisableCrtc(uint32_t crtc_id) {
  current_framebuffer_ = 0;
  return true;
}

ScopedDrmConnectorPtr MockDrmDevice::GetConnector(uint32_t connector_id) {
  return ScopedDrmConnectorPtr(DrmAllocator<drmModeConnector>());
}

bool MockDrmDevice::AddFramebuffer2(uint32_t width,
                                    uint32_t height,
                                    uint32_t format,
                                    uint32_t handles[4],
                                    uint32_t strides[4],
                                    uint32_t offsets[4],
                                    uint64_t modifiers[4],
                                    uint32_t* framebuffer,
                                    uint32_t flags) {
  add_framebuffer_call_count_++;
  *framebuffer = add_framebuffer_call_count_;
  return add_framebuffer_expectation_;
}

bool MockDrmDevice::RemoveFramebuffer(uint32_t framebuffer) {
  remove_framebuffer_call_count_++;
  return true;
}

ScopedDrmFramebufferPtr MockDrmDevice::GetFramebuffer(uint32_t framebuffer) {
  return ScopedDrmFramebufferPtr();
}

bool MockDrmDevice::PageFlip(uint32_t crtc_id,
                             uint32_t framebuffer,
                             PageFlipCallback callback) {
  page_flip_call_count_++;
  current_framebuffer_ = framebuffer;
  if (page_flip_expectation_) {
    if (use_sync_flips_)
      std::move(callback).Run(0, base::TimeTicks());
    else
      callbacks_.push(std::move(callback));
  }

  return page_flip_expectation_;
}

bool MockDrmDevice::PageFlipOverlay(uint32_t crtc_id,
                                    uint32_t framebuffer,
                                    const gfx::Rect& location,
                                    const gfx::Rect& source,
                                    int overlay_plane) {
  if (!framebuffer)
    overlay_clear_call_count_++;
  overlay_flip_call_count_++;
  return true;
}

ScopedDrmPropertyPtr MockDrmDevice::GetProperty(drmModeConnector* connector,
                                                const char* name) {
  return ScopedDrmPropertyPtr(DrmAllocator<drmModePropertyRes>());
}

ScopedDrmPropertyPtr MockDrmDevice::GetProperty(uint32_t id) {
  return ScopedDrmPropertyPtr(DrmAllocator<drmModePropertyRes>());
}

bool MockDrmDevice::SetProperty(uint32_t connector_id,
                                uint32_t property_id,
                                uint64_t value) {
  return true;
}

bool MockDrmDevice::GetCapability(uint64_t capability, uint64_t* value) {
  return true;
}

ScopedDrmPropertyBlobPtr MockDrmDevice::GetPropertyBlob(
    drmModeConnector* connector,
    const char* name) {
  return ScopedDrmPropertyBlobPtr(DrmAllocator<drmModePropertyBlobRes>());
}

bool MockDrmDevice::SetCursor(uint32_t crtc_id,
                              uint32_t handle,
                              const gfx::Size& size) {
  crtc_cursor_map_[crtc_id] = handle;
  return true;
}

bool MockDrmDevice::MoveCursor(uint32_t crtc_id, const gfx::Point& point) {
  return true;
}

bool MockDrmDevice::CreateDumbBuffer(const SkImageInfo& info,
                                     uint32_t* handle,
                                     uint32_t* stride) {
  if (!create_dumb_buffer_expectation_)
    return false;

  *handle = allocate_buffer_count_++;
  *stride = info.minRowBytes();
  void* pixels = new char[info.computeByteSize(*stride)];
  buffers_.push_back(SkSurface::MakeRasterDirect(info, pixels, *stride));
  buffers_[*handle]->getCanvas()->clear(SK_ColorBLACK);

  return true;
}

bool MockDrmDevice::DestroyDumbBuffer(uint32_t handle) {
  if (handle >= buffers_.size() || !buffers_[handle])
    return false;

  buffers_[handle].reset();
  return true;
}

bool MockDrmDevice::MapDumbBuffer(uint32_t handle, size_t size, void** pixels) {
  if (handle >= buffers_.size() || !buffers_[handle])
    return false;

  SkPixmap pixmap;
  buffers_[handle]->peekPixels(&pixmap);
  *pixels = const_cast<void*>(pixmap.addr());
  return true;
}

bool MockDrmDevice::UnmapDumbBuffer(void* pixels, size_t size) {
  return true;
}

bool MockDrmDevice::CloseBufferHandle(uint32_t handle) {
  return true;
}

bool MockDrmDevice::CommitProperties(drmModeAtomicReq* properties,
                                     uint32_t flags,
                                     uint32_t crtc_count,
                                     PageFlipCallback callback) {
  return false;
}

bool MockDrmDevice::SetGammaRamp(
    uint32_t crtc_id,
    const std::vector<display::GammaRampRGBEntry>& lut) {
  return true;
}

bool MockDrmDevice::SetCapability(uint64_t capability, uint64_t value) {
  return false;
}

void MockDrmDevice::RunCallbacks() {
  while (!callbacks_.empty()) {
    PageFlipCallback callback = std::move(callbacks_.front());
    callbacks_.pop();
    std::move(callback).Run(0, base::TimeTicks());
  }
}

}  // namespace ui
