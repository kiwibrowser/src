// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DBUS_FINCH_FEATURES_SERVICE_PROVIDER_DELEGATE_H_
#define CHROME_BROWSER_CHROMEOS_DBUS_FINCH_FEATURES_SERVICE_PROVIDER_DELEGATE_H_

#include "base/macros.h"
#include "chromeos/dbus/services/chrome_features_service_provider.h"

namespace chromeos {

// Finch implementation of ChromeFeaturesServiceProvider::Delegate.
class FinchFeaturesServiceProviderDelegate
    : public ChromeFeaturesServiceProvider::Delegate {
 public:
  FinchFeaturesServiceProviderDelegate();
  ~FinchFeaturesServiceProviderDelegate() override;

  // ChromeServiceProvider::Delegate:
  bool IsCrostiniEnabled() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FinchFeaturesServiceProviderDelegate);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_DBUS_FINCH_FEATURES_SERVICE_PROVIDER_DELEGATE_H_
