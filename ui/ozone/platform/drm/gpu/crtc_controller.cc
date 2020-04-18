// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/crtc_controller.h"

#include "base/logging.h"
#include "base/time/time.h"
#include "ui/gfx/presentation_feedback.h"
#include "ui/ozone/platform/drm/gpu/drm_device.h"
#include "ui/ozone/platform/drm/gpu/page_flip_request.h"
#include "ui/ozone/platform/drm/gpu/scanout_buffer.h"

namespace ui {

CrtcController::CrtcController(const scoped_refptr<DrmDevice>& drm,
                               uint32_t crtc,
                               uint32_t connector)
    : drm_(drm),
      crtc_(crtc),
      connector_(connector) {}

CrtcController::~CrtcController() {
  if (!is_disabled_) {
    const std::vector<std::unique_ptr<HardwareDisplayPlane>>& all_planes =
        drm_->plane_manager()->planes();
    for (const auto& plane : all_planes) {
      if (plane->owning_crtc() == crtc_) {
        plane->set_owning_crtc(0);
        plane->set_in_use(false);
      }
    }

    SetCursor(nullptr);
    drm_->DisableCrtc(crtc_);
    if (page_flip_request_) {
      // The controller is being destroyed, so signal the pending page flip
      // request with an empty presentation feedback which means the buffer
      // is never presented on a screen.
      SignalPageFlipRequest(gfx::SwapResult::SWAP_ACK,
                            gfx::PresentationFeedback());
    }
  }
}

bool CrtcController::Modeset(const OverlayPlane& plane, drmModeModeInfo mode) {
  if (!drm_->SetCrtc(crtc_, plane.buffer->GetOpaqueFramebufferId(),
                     std::vector<uint32_t>(1, connector_), &mode)) {
    PLOG(ERROR) << "Failed to modeset: crtc=" << crtc_
                << " connector=" << connector_
                << " framebuffer_id=" << plane.buffer->GetOpaqueFramebufferId()
                << " mode=" << mode.hdisplay << "x" << mode.vdisplay << "@"
                << mode.vrefresh;
    return false;
  }

  mode_ = mode;
  pending_planes_.clear();
  is_disabled_ = false;

  // drmModeSetCrtc has an immediate effect, so we can assume that the current
  // planes have been updated. However if a page flip is still pending, set the
  // pending planes to the same values so that the callback keeps the correct
  // state.
  current_planes_ = std::vector<OverlayPlane>(1, plane);
  if (page_flip_request_.get())
    pending_planes_ = current_planes_;

  ResetCursor();

  return true;
}

bool CrtcController::Disable() {
  if (is_disabled_)
    return true;

  is_disabled_ = true;
  return drm_->DisableCrtc(crtc_);
}

bool CrtcController::SchedulePageFlip(
    HardwareDisplayPlaneList* plane_list,
    const OverlayPlaneList& overlays,
    bool test_only,
    scoped_refptr<PageFlipRequest> page_flip_request) {
  DCHECK(!page_flip_request_.get() || test_only);
  DCHECK(!is_disabled_);

  const OverlayPlane* primary = OverlayPlane::GetPrimaryPlane(overlays);
  if (primary && !drm_->plane_manager()->ValidatePrimarySize(*primary, mode_)) {
    VLOG(2) << "Trying to pageflip a buffer with the wrong size. Expected "
            << mode_.hdisplay << "x" << mode_.vdisplay << " got "
            << primary->buffer->GetSize().ToString() << " for"
            << " crtc=" << crtc_ << " connector=" << connector_;
    page_flip_request->Signal(gfx::SwapResult::SWAP_ACK,
                              gfx::PresentationFeedback());
    return true;
  }

  if (!drm_->plane_manager()->AssignOverlayPlanes(plane_list, overlays, crtc_,
                                                  this)) {
    PLOG(ERROR) << "Failed to assign overlay planes for crtc " << crtc_;
    page_flip_request->Signal(gfx::SwapResult::SWAP_FAILED,
                              gfx::PresentationFeedback());
    return false;
  }

  if (test_only) {
    // If |test_only| is true, we just verify overlay configuration and don't
    // present anything on a screen, so signal the request with an empty
    // presentation feedback.
    page_flip_request->Signal(gfx::SwapResult::SWAP_ACK,
                              gfx::PresentationFeedback());
  } else {
    pending_planes_ = overlays;
    page_flip_request_ = page_flip_request;
  }

  return true;
}

std::vector<uint64_t> CrtcController::GetFormatModifiers(uint32_t format) {
  return drm_->plane_manager()->GetFormatModifiers(crtc_, format);
}

void CrtcController::OnPageFlipEvent(unsigned int frame,
                                     base::TimeTicks timestamp) {
  time_of_last_flip_ = timestamp;
  // For Ozone DRM, the page flip is aligned with VSYNC, and the timestamp is
  // provided by kernel DRM driver (kHWClock) and the buffer has been presented
  // on the screen (kHWCompletion).
  const uint32_t kFlags = gfx::PresentationFeedback::Flags::kVSync |
                          gfx::PresentationFeedback::Flags::kHWClock |
                          gfx::PresentationFeedback::Flags::kHWCompletion;
  SignalPageFlipRequest(
      gfx::SwapResult::SWAP_ACK,
      gfx::PresentationFeedback(
          time_of_last_flip_,
          mode_.vrefresh ? base::TimeDelta::FromSeconds(1) / mode_.vrefresh
                         : base::TimeDelta(),
          kFlags));
}

bool CrtcController::SetCursor(const scoped_refptr<ScanoutBuffer>& buffer) {
  DCHECK(!is_disabled_ || !buffer);
  cursor_buffer_ = buffer;

  return ResetCursor();
}

bool CrtcController::MoveCursor(const gfx::Point& location) {
  DCHECK(!is_disabled_);
  return drm_->MoveCursor(crtc_, location);
}

bool CrtcController::ResetCursor() {
  uint32_t handle = 0;
  gfx::Size size;

  if (cursor_buffer_) {
    handle = cursor_buffer_->GetHandle();
    size = cursor_buffer_->GetSize();
  }

  bool status = drm_->SetCursor(crtc_, handle, size);
  if (!status) {
    PLOG(ERROR) << "drmModeSetCursor: device " << drm_->device_path().value()
                << " crtc " << crtc_ << " handle " << handle << " size "
                << size.ToString();
  }

  return status;
}

void CrtcController::SignalPageFlipRequest(
    gfx::SwapResult result,
    const gfx::PresentationFeedback& feedback) {
  if (result == gfx::SwapResult::SWAP_ACK)
    current_planes_.swap(pending_planes_);

  pending_planes_.clear();

  scoped_refptr<PageFlipRequest> request;
  request.swap(page_flip_request_);
  request->Signal(result, feedback);
}

}  // namespace ui
