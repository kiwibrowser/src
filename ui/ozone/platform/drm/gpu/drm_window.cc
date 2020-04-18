// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/drm_window.h"

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "ui/gfx/presentation_feedback.h"
#include "ui/ozone/common/gpu/ozone_gpu_message_params.h"
#include "ui/ozone/platform/drm/common/drm_util.h"
#include "ui/ozone/platform/drm/gpu/crtc_controller.h"
#include "ui/ozone/platform/drm/gpu/drm_buffer.h"
#include "ui/ozone/platform/drm/gpu/drm_device.h"
#include "ui/ozone/platform/drm/gpu/drm_device_manager.h"
#include "ui/ozone/platform/drm/gpu/drm_overlay_validator.h"
#include "ui/ozone/platform/drm/gpu/screen_manager.h"

namespace ui {

namespace {

void UpdateCursorImage(DrmBuffer* cursor, const SkBitmap& image) {
  SkRect damage;
  image.getBounds(&damage);

  // Clear to transparent in case |image| is smaller than the canvas.
  SkCanvas* canvas = cursor->GetCanvas();
  canvas->clear(SK_ColorTRANSPARENT);
  canvas->drawBitmapRect(image, damage, NULL);
}

}  // namespace

DrmWindow::DrmWindow(gfx::AcceleratedWidget widget,
                     DrmDeviceManager* device_manager,
                     ScreenManager* screen_manager)
    : widget_(widget),
      device_manager_(device_manager),
      screen_manager_(screen_manager) {
}

DrmWindow::~DrmWindow() {
}

void DrmWindow::Initialize(ScanoutBufferGenerator* buffer_generator) {
  TRACE_EVENT1("drm", "DrmWindow::Initialize", "widget", widget_);

  device_manager_->UpdateDrmDevice(widget_, nullptr);
  overlay_validator_ =
      std::make_unique<DrmOverlayValidator>(this, buffer_generator);
}

void DrmWindow::Shutdown() {
  TRACE_EVENT1("drm", "DrmWindow::Shutdown", "widget", widget_);
  device_manager_->RemoveDrmDevice(widget_);
}

gfx::AcceleratedWidget DrmWindow::GetAcceleratedWidget() {
  return widget_;
}

HardwareDisplayController* DrmWindow::GetController() {
  return controller_;
}

void DrmWindow::SetBounds(const gfx::Rect& bounds) {
  TRACE_EVENT2("drm", "DrmWindow::SetBounds", "widget", widget_, "bounds",
               bounds.ToString());
  if (bounds_.size() != bounds.size())
    last_submitted_planes_.clear();

  bounds_ = bounds;
  screen_manager_->UpdateControllerToWindowMapping();
}

void DrmWindow::SetCursor(const std::vector<SkBitmap>& bitmaps,
                          const gfx::Point& location,
                          int frame_delay_ms) {
  cursor_bitmaps_ = bitmaps;
  cursor_location_ = location;
  cursor_frame_ = 0;
  cursor_frame_delay_ms_ = frame_delay_ms;
  cursor_timer_.Stop();

  if (cursor_frame_delay_ms_)
    cursor_timer_.Start(
        FROM_HERE, base::TimeDelta::FromMilliseconds(cursor_frame_delay_ms_),
        this, &DrmWindow::OnCursorAnimationTimeout);

  ResetCursor(false);
}

void DrmWindow::SetCursorWithoutAnimations(const std::vector<SkBitmap>& bitmaps,
                                           const gfx::Point& location) {
  cursor_bitmaps_ = bitmaps;
  cursor_location_ = location;
  cursor_frame_ = 0;
  cursor_frame_delay_ms_ = 0;
  ResetCursor(false);
}

void DrmWindow::MoveCursor(const gfx::Point& location) {
  cursor_location_ = location;

  if (controller_)
    controller_->MoveCursor(location);
}

bool DrmWindow::SchedulePageFlip(const std::vector<OverlayPlane>& planes,
                                 SwapCompletionOnceCallback callback) {
  if (controller_) {
    const DrmDevice* drm = controller_->GetAllocationDrmDevice().get();
    for (const auto& plane : planes) {
      if (plane.buffer && plane.buffer->GetDrmDevice() != drm) {
        // Although |force_buffer_reallocation_| is set to true during window
        // bounds update, this may still be needed because of in-flight buffers.
        force_buffer_reallocation_ = true;
        break;
      }
    }
  }

  if (force_buffer_reallocation_) {
    force_buffer_reallocation_ = false;
    std::move(callback).Run(gfx::SwapResult::SWAP_NAK_RECREATE_BUFFERS,
                            gfx::PresentationFeedback());
    return true;
  }

  last_submitted_planes_ = planes;

  if (!controller_) {
    std::move(callback).Run(gfx::SwapResult::SWAP_ACK,
                            gfx::PresentationFeedback());
    return true;
  }

  return controller_->SchedulePageFlip(last_submitted_planes_,
                                       std::move(callback));
}

std::vector<OverlayCheckReturn_Params> DrmWindow::TestPageFlip(
    const std::vector<OverlayCheck_Params>& overlay_params) {
  return overlay_validator_->TestPageFlip(overlay_params,
                                          last_submitted_planes_);
}

const OverlayPlane* DrmWindow::GetLastModesetBuffer() {
  return OverlayPlane::GetPrimaryPlane(last_submitted_planes_);
}

void DrmWindow::GetVSyncParameters(
    const gfx::VSyncProvider::UpdateVSyncCallback& callback) const {
  if (!controller_)
    return;

  // If we're in mirror mode the 2 CRTCs should have similar modes with the same
  // refresh rates.
  CrtcController* crtc = controller_->crtc_controllers()[0].get();
  const base::TimeTicks last_flip = controller_->GetTimeOfLastFlip();
  if (last_flip == base::TimeTicks() || crtc->mode().vrefresh == 0)
    return;  // The value is invalid, so we can't update the parameters.
  callback.Run(last_flip,
               base::TimeDelta::FromSeconds(1) / crtc->mode().vrefresh);
}

void DrmWindow::ResetCursor(bool bitmap_only) {
  if (!controller_)
    return;

  if (cursor_bitmaps_.size()) {
    // Draw new cursor into backbuffer.
    UpdateCursorImage(cursor_buffers_[cursor_frontbuffer_ ^ 1].get(),
                      cursor_bitmaps_[cursor_frame_]);

    // Reset location & buffer.
    if (!bitmap_only)
      controller_->MoveCursor(cursor_location_);
    controller_->SetCursor(cursor_buffers_[cursor_frontbuffer_ ^ 1]);
    cursor_frontbuffer_ ^= 1;
  } else {
    // No cursor set.
    controller_->UnsetCursor();
  }
}

void DrmWindow::OnCursorAnimationTimeout() {
  cursor_frame_++;
  cursor_frame_ %= cursor_bitmaps_.size();

  ResetCursor(true);
}

void DrmWindow::SetController(HardwareDisplayController* controller) {
  if (controller_ == controller)
    return;

  // Force buffer reallocation since the window moved to a different controller.
  // This is required otherwise the GPU will eventually try to render into the
  // buffer currently showing on the old controller (there is no guarantee that
  // the old controller has been updated in the meantime).
  force_buffer_reallocation_ = true;

  controller_ = controller;
  device_manager_->UpdateDrmDevice(
      widget_, controller ? controller->GetAllocationDrmDevice() : nullptr);

  UpdateCursorBuffers();
  // We changed displays, so we want to update the cursor as well.
  ResetCursor(false /* bitmap_only */);
}

void DrmWindow::UpdateCursorBuffers() {
  TRACE_EVENT0("drm", "DrmWindow::UpdateCursorBuffers");
  if (!controller_) {
    for (size_t i = 0; i < arraysize(cursor_buffers_); ++i) {
      cursor_buffers_[i] = nullptr;
    }
  } else {
    scoped_refptr<DrmDevice> drm = controller_->GetAllocationDrmDevice();
    gfx::Size max_cursor_size = GetMaximumCursorSize(drm->get_fd());
    SkImageInfo info = SkImageInfo::MakeN32Premul(max_cursor_size.width(),
                                                  max_cursor_size.height());
    for (size_t i = 0; i < arraysize(cursor_buffers_); ++i) {
      cursor_buffers_[i] = new DrmBuffer(drm);
      // Don't register a framebuffer for cursors since they are special (they
      // aren't modesetting buffers and drivers may fail to register them due to
      // their small sizes).
      if (!cursor_buffers_[i]->Initialize(
              info, false /* should_register_framebuffer */)) {
        LOG(FATAL) << "Failed to initialize cursor buffer";
        return;
      }
    }
  }
}

}  // namespace ui
