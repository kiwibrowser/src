// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_DRM_GPU_UTIL_H_
#define UI_OZONE_PLATFORM_DRM_GPU_DRM_GPU_UTIL_H_

#include "ui/ozone/platform/drm/common/scoped_drm_types.h"
#include "ui/ozone/platform/drm/gpu/drm_device.h"

namespace ui {

// Helper function that finds the property with the specified name.
bool GetDrmPropertyForName(DrmDevice* drm,
                           drmModeObjectProperties* properties,
                           const std::string& name,
                           DrmDevice::Property* property);

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_DRM_GPU_UTIL_H_
