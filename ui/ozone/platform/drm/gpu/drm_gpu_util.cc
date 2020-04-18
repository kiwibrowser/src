// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/drm_gpu_util.h"

#include <xf86drmMode.h>

#include "ui/ozone/platform/drm/gpu/drm_device.h"

namespace ui {

bool GetDrmPropertyForName(DrmDevice* drm,
                           drmModeObjectProperties* properties,
                           const std::string& name,
                           DrmDevice::Property* property) {
  for (uint32_t i = 0; i < properties->count_props; ++i) {
    ScopedDrmPropertyPtr drm_property(drm->GetProperty(properties->props[i]));
    if (name != drm_property->name)
      continue;

    property->id = drm_property->prop_id;
    property->value = properties->prop_values[i];
    if (property->id)
      return true;
  }

  return false;
}

}  // namespace ui
