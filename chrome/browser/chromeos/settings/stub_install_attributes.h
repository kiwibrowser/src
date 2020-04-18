// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SETTINGS_STUB_INSTALL_ATTRIBUTES_H_
#define CHROME_BROWSER_CHROMEOS_SETTINGS_STUB_INSTALL_ATTRIBUTES_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/chromeos/settings/install_attributes.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"

namespace chromeos {

// This class allows tests to set specific configurations for testing.
class StubInstallAttributes : public InstallAttributes {
 public:
  StubInstallAttributes();

  // Setup as not-yet enrolled.
  void Clear();

  // Setup as consumer owned device. (Clears existing configuration.)
  void SetConsumerOwned();

  // Setup as managed by Google cloud. (Clears existing configuration.)
  void SetCloudManaged(const std::string& domain, const std::string& device_id);

  // Setup as managed by Active Directory server. (Clears existing
  // configuration.)
  void SetActiveDirectoryManaged(const std::string& realm,
                                 const std::string& device_id);

 private:
  DISALLOW_COPY_AND_ASSIGN(StubInstallAttributes);
};

// Helper class to set install attributes in tests. Using one of the Create*
// methods injects the generated StubInstallAttributes into the next
// instantiation of BrowserPolicyConnectorChromeOS. Scoping ensures that the
// generated StubInstallAttributes are not leaked in case there is no
// instantiation of BrowserPolicyConnectorChromeOS.
class ScopedStubInstallAttributes {
 public:
  // Setting |cleanup_| to false in the moved-from object ensures that the
  // StubInstallAttributes are not deleted prematurely when the return value of
  // a Create* method is move-assigned to the scoping variable.
  ScopedStubInstallAttributes(ScopedStubInstallAttributes&& other) {
    cleanup_ = other.cleanup_;
    other.cleanup_ = false;
  }

  ~ScopedStubInstallAttributes();

  // Factory for empty (unset) ScopedStubInstallAttributes.
  static ScopedStubInstallAttributes CreateUnset();

  // Factory for consumer-type ScopedStubInstallAttributes.
  static ScopedStubInstallAttributes CreateConsumerOwned();

  // Factory for cloud managed ScopedStubInstallAttributes.
  static ScopedStubInstallAttributes CreateCloudManaged(
      const std::string& domain,
      const std::string& device_id);

  // Factory for Active Directory managed ScopedStubInstallAttributes.
  static ScopedStubInstallAttributes CreateActiveDirectoryManaged(
      const std::string& realm,
      const std::string& device_id);

 private:
  ScopedStubInstallAttributes() = default;

  // Whether the destructor should call into BrowserPolicyConnectorChromeOS to
  // delete the StubInstallAttributes.
  bool cleanup_ = true;

  DISALLOW_COPY_AND_ASSIGN(ScopedStubInstallAttributes);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SETTINGS_STUB_INSTALL_ATTRIBUTES_H_
