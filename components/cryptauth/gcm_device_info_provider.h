// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_GCM_DEVICE_INFO_PROVIDER_H_
#define COMPONENTS_CRYPTAUTH_GCM_DEVICE_INFO_PROVIDER_H_

#include "base/macros.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace cryptauth {

// Provides the GcmDeviceInfo object associated with the current device.
// GcmDeviceInfo describes properties of this Chromebook and is not expected to
// change except when the OS version is updated.
class GcmDeviceInfoProvider {
 public:
  GcmDeviceInfoProvider() = default;
  virtual ~GcmDeviceInfoProvider() = default;

  virtual const GcmDeviceInfo& GetGcmDeviceInfo() const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(GcmDeviceInfoProvider);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_GCM_DEVICE_INFO_PROVIDER_H_
