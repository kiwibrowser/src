// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/overlay_info.h"
#include "media/base/surface_manager.h"

namespace media {

OverlayInfo::OverlayInfo() = default;
OverlayInfo::OverlayInfo(const OverlayInfo&) = default;

bool OverlayInfo::HasValidSurfaceId() const {
  return surface_id != SurfaceManager::kNoSurfaceID;
}

bool OverlayInfo::HasValidRoutingToken() const {
  return routing_token.has_value();
}

bool OverlayInfo::RefersToSameOverlayAs(const OverlayInfo& other) {
  return surface_id == other.surface_id && routing_token == other.routing_token;
}

}  // namespace media
