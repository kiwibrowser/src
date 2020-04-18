// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CRYPTAUTH_GCM_DEVICE_INFO_PROVIDER_IMPL_H_
#define CHROME_BROWSER_CHROMEOS_CRYPTAUTH_GCM_DEVICE_INFO_PROVIDER_IMPL_H_

#include "base/macros.h"
#include "base/no_destructor.h"
#include "components/cryptauth/gcm_device_info_provider.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace chromeos {

// Concrete GcmDeviceInfoProvider implementation.
class GcmDeviceInfoProviderImpl : public cryptauth::GcmDeviceInfoProvider {
 public:
  static const GcmDeviceInfoProviderImpl* GetInstance();

  // cryptauth::GcmDeviceInfoProvider:
  const cryptauth::GcmDeviceInfo& GetGcmDeviceInfo() const override;

 private:
  friend class base::NoDestructor<GcmDeviceInfoProviderImpl>;

  GcmDeviceInfoProviderImpl();

  DISALLOW_COPY_AND_ASSIGN(GcmDeviceInfoProviderImpl);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CRYPTAUTH_GCM_DEVICE_INFO_PROVIDER_IMPL_H_
