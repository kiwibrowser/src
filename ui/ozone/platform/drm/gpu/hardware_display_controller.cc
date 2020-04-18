// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/hardware_display_controller.h"

#include <drm.h>
#include <string.h>
#include <xf86drm.h>
#include <utility>

#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "third_party/libdrm/src/include/drm/drm_fourcc.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/native_pixmap.h"
#include "ui/gfx/presentation_feedback.h"
#include "ui/gfx/swap_result.h"
#include "ui/ozone/platform/drm/gpu/crtc_controller.h"
#include "ui/ozone/platform/drm/gpu/drm_buffer.h"
#include "ui/ozone/platform/drm/gpu/drm_device.h"
#include "ui/ozone/platform/drm/gpu/page_flip_request.h"

namespace ui {

namespace {

void EmptyFlipCallback(gfx::SwapResult,
                       const gfx::PresentationFeedback& feedback) {}

}  // namespace

HardwareDisplayController::HardwareDisplayController(
    std::unique_ptr<CrtcController> controller,
    const gfx::Point& origin)
    : origin_(origin), is_disabled_(controller->is_disabled()) {
  AddCrtc(std::move(controller));
}

HardwareDisplayController::~HardwareDisplayController() {
  // Reset the cursor.
  UnsetCursor();
}

bool HardwareDisplayController::Modeset(const OverlayPlane& primary,
                                        drmModeModeInfo mode) {
  TRACE_EVENT0("drm", "HDC::Modeset");
  DCHECK(primary.buffer.get());
  bool status = true;
  for (const auto& controller : crtc_controllers_)
    status &= controller->Modeset(primary, mode);

  is_disabled_ = false;

  return status;
}

bool HardwareDisplayController::Enable(const OverlayPlane& primary) {
  TRACE_EVENT0("drm", "HDC::Enable");
  DCHECK(primary.buffer.get());
  bool status = true;
  for (const auto& controller : crtc_controllers_)
    status &= controller->Modeset(primary, controller->mode());

  is_disabled_ = false;

  return status;
}

void HardwareDisplayController::Disable() {
  TRACE_EVENT0("drm", "HDC::Disable");
  for (const auto& controller : crtc_controllers_)
    controller->Disable();

  for (const auto& planes : owned_hardware_planes_) {
    DrmDevice* drm = planes.first;
    HardwareDisplayPlaneList* plane_list = planes.second.get();
    bool ret = drm->plane_manager()->DisableOverlayPlanes(plane_list);
    LOG_IF(ERROR, !ret) << "Can't disable overlays when disabling HDC.";
  }

  is_disabled_ = true;
}

bool HardwareDisplayController::SchedulePageFlip(
    const OverlayPlaneList& plane_list,
    SwapCompletionOnceCallback callback) {
  return ActualSchedulePageFlip(plane_list, false /* test_only */,
                                std::move(callback));
}

bool HardwareDisplayController::TestPageFlip(
    const OverlayPlaneList& plane_list) {
  return ActualSchedulePageFlip(plane_list, true /* test_only */,
                                base::BindOnce(&EmptyFlipCallback));
}

bool HardwareDisplayController::ActualSchedulePageFlip(
    const OverlayPlaneList& plane_list,
    bool test_only,
    SwapCompletionOnceCallback callback) {
  TRACE_EVENT0("drm", "HDC::SchedulePageFlip");

  DCHECK(!is_disabled_);

  // Ignore requests with no planes to schedule.
  if (plane_list.empty()) {
    std::move(callback).Run(gfx::SwapResult::SWAP_ACK,
                            gfx::PresentationFeedback());
    return true;
  }

  OverlayPlaneList pending_planes = plane_list;
  std::sort(pending_planes.begin(), pending_planes.end(),
            [](const OverlayPlane& l, const OverlayPlane& r) {
              return l.z_order < r.z_order;
            });
  scoped_refptr<PageFlipRequest> page_flip_request =
      new PageFlipRequest(crtc_controllers_.size(), std::move(callback));

  for (const auto& planes : owned_hardware_planes_)
    planes.first->plane_manager()->BeginFrame(planes.second.get());

  bool status = true;
  for (const auto& controller : crtc_controllers_) {
    status &= controller->SchedulePageFlip(
        owned_hardware_planes_[controller->drm().get()].get(), pending_planes,
        test_only, page_flip_request);
  }

  for (const auto& planes : owned_hardware_planes_) {
    if (!planes.first->plane_manager()->Commit(planes.second.get(),
                                               test_only)) {
      status = false;
    }
  }

  return status;
}

std::vector<uint64_t> HardwareDisplayController::GetFormatModifiers(
    uint32_t format) {
  std::vector<uint64_t> modifiers;

  if (crtc_controllers_.empty())
    return modifiers;

  modifiers = crtc_controllers_[0]->GetFormatModifiers(format);

  for (size_t i = 1; i < crtc_controllers_.size(); ++i) {
    std::vector<uint64_t> other =
        crtc_controllers_[i]->GetFormatModifiers(format);
    std::vector<uint64_t> intersection;

    std::set_intersection(modifiers.begin(), modifiers.end(), other.begin(),
                          other.end(), std::back_inserter(intersection));
    modifiers = std::move(intersection);
  }

  return modifiers;
}

std::vector<uint64_t>
HardwareDisplayController::GetFormatModifiersForModesetting(
    uint32_t fourcc_format) {
  const auto& modifiers = GetFormatModifiers(fourcc_format);
  std::vector<uint64_t> filtered_modifiers;
  for (auto modifier : modifiers) {
    // AFBC for modeset buffers doesn't work correctly, as we can't fill it with
    // a valid AFBC buffer. For now, don't use AFBC for modeset buffers.
    // TODO: Use AFBC for modeset buffers if it is available.
    // See https://crbug.com/852675.
    if (modifier != DRM_FORMAT_MOD_CHROMEOS_ROCKCHIP_AFBC) {
      filtered_modifiers.push_back(modifier);
    }
  }
  return filtered_modifiers;
}

bool HardwareDisplayController::SetCursor(
    const scoped_refptr<ScanoutBuffer>& buffer) {
  bool status = true;

  if (is_disabled_)
    return true;

  for (const auto& controller : crtc_controllers_)
    status &= controller->SetCursor(buffer);

  return status;
}

bool HardwareDisplayController::UnsetCursor() {
  bool status = true;
  for (const auto& controller : crtc_controllers_)
    status &= controller->SetCursor(nullptr);

  return status;
}

bool HardwareDisplayController::MoveCursor(const gfx::Point& location) {
  if (is_disabled_)
    return true;

  bool status = true;
  for (const auto& controller : crtc_controllers_)
    status &= controller->MoveCursor(location);

  return status;
}

void HardwareDisplayController::AddCrtc(
    std::unique_ptr<CrtcController> controller) {
  scoped_refptr<DrmDevice> drm = controller->drm();

  std::unique_ptr<HardwareDisplayPlaneList>& owned_planes =
      owned_hardware_planes_[drm.get()];
  if (!owned_planes)
    owned_planes.reset(new HardwareDisplayPlaneList());

  // Check if this controller owns any planes and ensure we keep track of them.
  const std::vector<std::unique_ptr<HardwareDisplayPlane>>& all_planes =
      drm->plane_manager()->planes();
  HardwareDisplayPlaneList* crtc_plane_list = owned_planes.get();
  uint32_t crtc = controller->crtc();
  for (const auto& plane : all_planes) {
    if (plane->in_use() && (plane->owning_crtc() == crtc))
      crtc_plane_list->old_plane_list.push_back(plane.get());
  }

  crtc_controllers_.push_back(std::move(controller));
}

std::unique_ptr<CrtcController> HardwareDisplayController::RemoveCrtc(
    const scoped_refptr<DrmDevice>& drm,
    uint32_t crtc) {
  auto controller_it = std::find_if(
      crtc_controllers_.begin(), crtc_controllers_.end(),
      [drm, crtc](const std::unique_ptr<CrtcController>& crtc_controller) {
        return crtc_controller->drm() == drm && crtc_controller->crtc() == crtc;
      });
  if (controller_it == crtc_controllers_.end())
    return nullptr;

  std::unique_ptr<CrtcController> controller(std::move(*controller_it));
  crtc_controllers_.erase(controller_it);

  // Remove and disable only the planes owned by the CRTC we just
  // removed.
  std::vector<HardwareDisplayPlane*>& old_plane_list =
      owned_hardware_planes_[drm.get()]->old_plane_list;

  // Move all the planes that have been committed in the last pageflip for this
  // CRTC at the end of the collection.
  auto first_plane_to_disable_it =
      std::partition(old_plane_list.begin(), old_plane_list.end(),
                     [crtc](const HardwareDisplayPlane* plane) {
                       return plane->owning_crtc() != crtc;
                     });

  // Disable the planes enabled with the last commit on |crtc|, otherwise
  // the planes will be visible if the crtc is reassigned to another connector.
  HardwareDisplayPlaneList hardware_plane_list;
  std::copy(first_plane_to_disable_it, old_plane_list.end(),
            std::back_inserter(hardware_plane_list.old_plane_list));
  drm->plane_manager()->DisableOverlayPlanes(&hardware_plane_list);

  // If it was the only CRTC for this drm device, we can remove the hardware
  // planes list in |owned_hardware_planes_|.
  if (std::find_if(crtc_controllers_.begin(), crtc_controllers_.end(),
                   [drm](const std::unique_ptr<CrtcController>& crtc) {
                     return crtc->drm() == drm;
                   }) == crtc_controllers_.end()) {
    owned_hardware_planes_.erase(controller->drm().get());
  } else {
    // Otherwise we can remove the planes assigned to |crtc| but we can't
    // remove the entry in |owned_hardware_planes_|.
    old_plane_list.erase(first_plane_to_disable_it, old_plane_list.end());
  }

  return controller;
}

bool HardwareDisplayController::HasCrtc(const scoped_refptr<DrmDevice>& drm,
                                        uint32_t crtc) const {
  for (const auto& controller : crtc_controllers_) {
    if (controller->drm() == drm && controller->crtc() == crtc)
      return true;
  }

  return false;
}

bool HardwareDisplayController::IsMirrored() const {
  return crtc_controllers_.size() > 1;
}

bool HardwareDisplayController::IsDisabled() const {
  return is_disabled_;
}

gfx::Size HardwareDisplayController::GetModeSize() const {
  // If there are multiple CRTCs they should all have the same size.
  return gfx::Size(crtc_controllers_[0]->mode().hdisplay,
                   crtc_controllers_[0]->mode().vdisplay);
}

base::TimeTicks HardwareDisplayController::GetTimeOfLastFlip() const {
  base::TimeTicks time;
  for (const auto& controller : crtc_controllers_) {
    if (time < controller->time_of_last_flip())
      time = controller->time_of_last_flip();
  }

  return time;
}

scoped_refptr<DrmDevice> HardwareDisplayController::GetAllocationDrmDevice()
    const {
  DCHECK(!crtc_controllers_.empty());
  // TODO(dnicoara) When we support mirroring across DRM devices, figure out
  // which device should be used for allocations.
  return crtc_controllers_[0]->drm();
}

}  // namespace ui
