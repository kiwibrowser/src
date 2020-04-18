// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/drm_device_generator.h"

#include <utility>

#include "ui/ozone/platform/drm/gpu/drm_device.h"

namespace ui {

DrmDeviceGenerator::DrmDeviceGenerator() {
}

DrmDeviceGenerator::~DrmDeviceGenerator() {
}

scoped_refptr<DrmDevice> DrmDeviceGenerator::CreateDevice(
    const base::FilePath& device_path,
    base::File file,
    bool is_primary_device) {
  scoped_refptr<DrmDevice> drm =
      new DrmDevice(device_path, std::move(file), is_primary_device);
  if (drm->Initialize())
    return drm;

  return nullptr;
}

}  // namespace ui
