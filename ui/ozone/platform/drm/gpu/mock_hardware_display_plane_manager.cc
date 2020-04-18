// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/mock_hardware_display_plane_manager.h"

#include <drm_fourcc.h>
#include <utility>

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/ozone/platform/drm/gpu/fake_plane_info.h"
#include "ui/ozone/platform/drm/gpu/mock_scanout_buffer.h"

namespace ui {

MockHardwareDisplayPlaneManager::MockHardwareDisplayPlaneManager(
    DrmDevice* drm,
    const std::vector<uint32_t>& crtcs,
    uint32_t planes_per_crtc) {
  const int kPlaneBaseId = 50;
  const struct drm_format_modifier linear_modifier { 0x1, DRM_FORMAT_MOD_NONE };
  drm_ = drm;
  for (size_t i = 0; i < crtcs.size(); ++i)
    crtc_properties_.push_back({.id = crtcs[i]});

  for (size_t crtc_idx = 0; crtc_idx < crtc_properties_.size(); crtc_idx++) {
    for (size_t i = 0; i < planes_per_crtc; i++) {
      std::unique_ptr<HardwareDisplayPlane> plane(
          new HardwareDisplayPlane(kPlaneBaseId + i, 1 << crtc_idx));
      plane->Initialize(drm, std::vector<uint32_t>(1, DRM_FORMAT_XRGB8888),
                        std::vector<drm_format_modifier>(1, linear_modifier),
                        false, true);
      plane->set_type(i ? HardwareDisplayPlane::kOverlay
                        : HardwareDisplayPlane::kPrimary);
      planes_.push_back(std::move(plane));
    }
  }

  // The real HDPM uses sorted planes, so sort them for consistency.
  std::sort(planes_.begin(), planes_.end(),
            [](const std::unique_ptr<HardwareDisplayPlane>& l,
               const std::unique_ptr<HardwareDisplayPlane>& r) {
              return l->plane_id() < r->plane_id();
            });
}

MockHardwareDisplayPlaneManager::MockHardwareDisplayPlaneManager(
    DrmDevice* drm) {
  drm_ = drm;
}

MockHardwareDisplayPlaneManager::~MockHardwareDisplayPlaneManager() {}

void MockHardwareDisplayPlaneManager::InitForTest(
    const FakePlaneInfo* planes,
    size_t count,
    const std::vector<uint32_t>& crtcs) {
  crtc_properties_.clear();
  for (size_t i = 0; i < crtcs.size(); ++i)
    crtc_properties_.push_back({.id = crtcs[i]});

  planes_.clear();
  for (size_t i = 0; i < count; i++) {
    std::unique_ptr<HardwareDisplayPlane> plane(
        new HardwareDisplayPlane(planes[i].id, planes[i].allowed_crtc_mask));
    plane->Initialize(drm_, planes[i].allowed_formats,
                      std::vector<drm_format_modifier>(),  // modifiers
                      false, true);
    planes_.push_back(std::move(plane));
  }
  // The real HDPM uses sorted planes, so sort them for consistency.
  std::sort(planes_.begin(), planes_.end(),
            [](const std::unique_ptr<HardwareDisplayPlane>& l,
               const std::unique_ptr<HardwareDisplayPlane>& r) {
              return l->plane_id() < r->plane_id();
            });
}

void MockHardwareDisplayPlaneManager::SetPlaneProperties(
    const std::vector<FakePlaneInfo>& planes) {
  planes_.clear();
  uint32_t count = planes.size();
  for (size_t i = 0; i < count; i++) {
    std::unique_ptr<HardwareDisplayPlane> plane(
        new HardwareDisplayPlane(planes[i].id, planes[i].allowed_crtc_mask));
    plane->Initialize(drm_, planes[i].allowed_formats,
                      std::vector<drm_format_modifier>(),  // modifiers
                      false, true);
    planes_.push_back(std::move(plane));
  }

  // The real HDPM uses sorted planes, so sort them for consistency.
  std::sort(planes_.begin(), planes_.end(),
            [](const std::unique_ptr<HardwareDisplayPlane>& l,
               const std::unique_ptr<HardwareDisplayPlane>& r) {
              return l->plane_id() < r->plane_id();
            });

  ResetPlaneCount();
}

void MockHardwareDisplayPlaneManager::SetCrtcInfo(
    const std::vector<uint32_t>& crtcs) {
  crtc_properties_.clear();
  for (size_t i = 0; i < crtcs.size(); ++i)
    crtc_properties_.push_back({.id = crtcs[i]});

  planes_.clear();
  ResetPlaneCount();
}

bool MockHardwareDisplayPlaneManager::DisableOverlayPlanes(
    HardwareDisplayPlaneList* plane_list) {
  for (HardwareDisplayPlane* plane : plane_list->old_plane_list) {
    if (plane->type() != HardwareDisplayPlane::kOverlay)
      continue;
    plane->set_in_use(false);
    plane->set_owning_crtc(0);
  }

  return true;
}

bool MockHardwareDisplayPlaneManager::SetPlaneData(
    HardwareDisplayPlaneList* plane_list,
    HardwareDisplayPlane* hw_plane,
    const OverlayPlane& overlay,
    uint32_t crtc_id,
    const gfx::Rect& src_rect,
    CrtcController* crtc) {
  // Check that the chosen plane is a legal choice for the crtc.
  EXPECT_NE(-1, LookupCrtcIndex(crtc_id));
  EXPECT_TRUE(hw_plane->CanUseForCrtc(LookupCrtcIndex(crtc_id)));
  EXPECT_FALSE(hw_plane->in_use());
  plane_count_++;
  return HardwareDisplayPlaneManagerLegacy::SetPlaneData(
      plane_list, hw_plane, overlay, crtc_id, src_rect, crtc);
}

int MockHardwareDisplayPlaneManager::plane_count() const {
  return plane_count_;
}

void MockHardwareDisplayPlaneManager::ResetPlaneCount() {
  plane_count_ = 0;
}

}  // namespace ui
