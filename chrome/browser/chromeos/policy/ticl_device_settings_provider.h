// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_TICL_DEVICE_SETTINGS_PROVIDER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_TICL_DEVICE_SETTINGS_PROVIDER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/invalidation/impl/ticl_settings_provider.h"

namespace policy {

// A specialization of TiclSettingsProvider to be used by a device-wide
// TiclInvalidationService.
class TiclDeviceSettingsProvider : public invalidation::TiclSettingsProvider {
 public:
  TiclDeviceSettingsProvider();
  ~TiclDeviceSettingsProvider() override;

  // TiclInvalidationServiceSettingsProvider:
  bool UseGCMChannel() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TiclDeviceSettingsProvider);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_TICL_DEVICE_SETTINGS_PROVIDER_H_
