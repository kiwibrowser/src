// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/fake_gcm_device_info_provider.h"

namespace cryptauth {

FakeGcmDeviceInfoProvider::FakeGcmDeviceInfoProvider(
    const GcmDeviceInfo& gcm_device_info)
    : gcm_device_info_(gcm_device_info) {}

FakeGcmDeviceInfoProvider::~FakeGcmDeviceInfoProvider() = default;

const GcmDeviceInfo& FakeGcmDeviceInfoProvider::GetGcmDeviceInfo() const {
  return gcm_device_info_;
}

}  // namespace cryptauth
