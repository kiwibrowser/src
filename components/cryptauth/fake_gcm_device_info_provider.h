// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_FAKE_GCM_DEVICE_INFO_PROVIDER_H_
#define COMPONENTS_CRYPTAUTH_FAKE_GCM_DEVICE_INFO_PROVIDER_H_

#include "base/macros.h"
#include "components/cryptauth/gcm_device_info_provider.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace cryptauth {

// Test GcmDeviceInfoProvider implementation.
class FakeGcmDeviceInfoProvider : public GcmDeviceInfoProvider {
 public:
  FakeGcmDeviceInfoProvider(const GcmDeviceInfo& gcm_device_info);
  ~FakeGcmDeviceInfoProvider() override;

  // GcmDeviceInfoProvider:
  const GcmDeviceInfo& GetGcmDeviceInfo() const override;

 private:
  const GcmDeviceInfo gcm_device_info_;

  DISALLOW_COPY_AND_ASSIGN(FakeGcmDeviceInfoProvider);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_FAKE_GCM_DEVICE_INFO_PROVIDER_H_
