// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/settings/stub_install_attributes.h"

#include <string>

#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"

namespace chromeos {

StubInstallAttributes::StubInstallAttributes() : InstallAttributes(nullptr) {
  device_locked_ = true;
}

void StubInstallAttributes::Clear() {
  registration_mode_ = policy::DEVICE_MODE_NOT_SET;
  registration_domain_.clear();
  registration_realm_.clear();
  registration_device_id_.clear();
}

void StubInstallAttributes::SetConsumerOwned() {
  registration_mode_ = policy::DEVICE_MODE_CONSUMER;
  registration_domain_.clear();
  registration_realm_.clear();
  registration_device_id_.clear();
}

void StubInstallAttributes::SetCloudManaged(const std::string& domain,
                                            const std::string& device_id) {
  registration_mode_ = policy::DEVICE_MODE_ENTERPRISE;
  registration_domain_ = domain;
  registration_realm_.clear();
  registration_device_id_ = device_id;
}

void StubInstallAttributes::SetActiveDirectoryManaged(
    const std::string& realm,
    const std::string& device_id) {
  registration_mode_ = policy::DEVICE_MODE_ENTERPRISE_AD;
  registration_realm_ = realm;
  registration_domain_.clear();
  registration_device_id_ = device_id;
}

// static
ScopedStubInstallAttributes ScopedStubInstallAttributes::CreateUnset() {
  StubInstallAttributes* attributes = new StubInstallAttributes();
  attributes->Clear();
  policy::BrowserPolicyConnectorChromeOS::SetInstallAttributesForTesting(
      attributes);
  return ScopedStubInstallAttributes();
}

// static
ScopedStubInstallAttributes ScopedStubInstallAttributes::CreateConsumerOwned() {
  StubInstallAttributes* attributes = new StubInstallAttributes();
  attributes->SetConsumerOwned();
  policy::BrowserPolicyConnectorChromeOS::SetInstallAttributesForTesting(
      attributes);
  return ScopedStubInstallAttributes();
}

// static
ScopedStubInstallAttributes ScopedStubInstallAttributes::CreateCloudManaged(
    const std::string& domain,
    const std::string& device_id) {
  StubInstallAttributes* attributes = new StubInstallAttributes();
  attributes->SetCloudManaged(domain, device_id);
  policy::BrowserPolicyConnectorChromeOS::SetInstallAttributesForTesting(
      attributes);
  return ScopedStubInstallAttributes();
}

// static
ScopedStubInstallAttributes
ScopedStubInstallAttributes::CreateActiveDirectoryManaged(
    const std::string& realm,
    const std::string& device_id) {
  StubInstallAttributes* attributes = new StubInstallAttributes();
  attributes->SetActiveDirectoryManaged(realm, device_id);
  policy::BrowserPolicyConnectorChromeOS::SetInstallAttributesForTesting(
      attributes);
  return ScopedStubInstallAttributes();
}

ScopedStubInstallAttributes::~ScopedStubInstallAttributes() {
  if (cleanup_)
    policy::BrowserPolicyConnectorChromeOS::RemoveInstallAttributesForTesting();
}

}  // namespace chromeos
