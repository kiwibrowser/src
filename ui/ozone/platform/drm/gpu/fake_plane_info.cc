// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <drm_fourcc.h>

#include "ui/ozone/platform/drm/gpu/fake_plane_info.h"

namespace ui {

FakePlaneInfo::FakePlaneInfo(uint32_t plane_id, uint32_t crtc_mask)
    : id(plane_id), allowed_crtc_mask(crtc_mask) {
  allowed_formats.push_back(DRM_FORMAT_XRGB8888);
}

FakePlaneInfo::FakePlaneInfo(uint32_t plane_id,
                             uint32_t crtc_mask,
                             const std::vector<uint32_t>& formats)
    : id(plane_id), allowed_crtc_mask(crtc_mask), allowed_formats(formats) {}

FakePlaneInfo::FakePlaneInfo(const FakePlaneInfo& other) = default;

FakePlaneInfo::~FakePlaneInfo() {}

}  // namespace ui
