// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/dbus/finch_features_service_provider_delegate.h"

#include "chrome/browser/chromeos/virtual_machines/virtual_machines_util.h"

namespace chromeos {

FinchFeaturesServiceProviderDelegate::FinchFeaturesServiceProviderDelegate() {}

FinchFeaturesServiceProviderDelegate::~FinchFeaturesServiceProviderDelegate() {}

bool FinchFeaturesServiceProviderDelegate::IsCrostiniEnabled() {
  return virtual_machines::AreVirtualMachinesAllowedByVersionAndChannel() &&
         virtual_machines::AreVirtualMachinesAllowedByPolicy();
}

}  // namespace chromeos
