// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/gbm_device.h"

#include <gbm.h>
#include <utility>

namespace ui {

GbmDevice::GbmDevice(const base::FilePath& device_path,
                     base::File file,
                     bool is_primary_device)
    : DrmDevice(device_path, std::move(file), is_primary_device) {}

GbmDevice::~GbmDevice() {
  if (device_)
    gbm_device_destroy(device_);
}

bool GbmDevice::Initialize() {
  if (!DrmDevice::Initialize())
    return false;

  device_ = gbm_create_device(get_fd());
  if (!device_) {
    PLOG(ERROR) << "Unable to initialize GBM for " << device_path().value();
    return false;
  }

  return true;
}

}  // namespace ui
