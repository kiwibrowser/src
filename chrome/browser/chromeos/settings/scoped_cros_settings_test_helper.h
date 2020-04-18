// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SETTINGS_SCOPED_CROS_SETTINGS_TEST_HELPER_H_
#define CHROME_BROWSER_CHROMEOS_SETTINGS_SCOPED_CROS_SETTINGS_TEST_HELPER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/chromeos/settings/stub_cros_settings_provider.h"
#include "chromeos/settings/cros_settings_provider.h"

class Profile;

namespace base {
class Value;
}

namespace chromeos {

class FakeOwnerSettingsService;
class ScopedTestCrosSettings;
class ScopedTestDeviceSettingsService;

class ScopedCrosSettingsTestHelper {
 public:
  // In some cases it is required to pass |create_settings_service| as false:
  // If the test already has a device settings service and/or CrosSettings set
  // up by another (instantiated or base) class, creating another one causes
  // crash.
  explicit ScopedCrosSettingsTestHelper(bool create_settings_service = true);
  ~ScopedCrosSettingsTestHelper();

  // Methods to replace and restore CrosSettingsProvider for the specified
  // |path|.
  void ReplaceProvider(const std::string& path);
  void RestoreProvider();

  // Method to create an owner settings service that uses
  // |stub_settings_provider_| as settings write path.
  std::unique_ptr<FakeOwnerSettingsService> CreateOwnerSettingsService(
      Profile* profile);

  // These methods simply call the according |stub_settings_provider_| method.
  void SetTrustedStatus(CrosSettingsProvider::TrustedStatus status);
  void SetCurrentUserIsOwner(bool owner);
  void Set(const std::string& path, const base::Value& in_value);

  // Convenience forms of Set() from CrosSettingsProvider. These methods will
  // replace any existing value at that |path|, even if it has a different type.
  void SetBoolean(const std::string& path, bool in_value);
  void SetInteger(const std::string& path, int in_value);
  void SetDouble(const std::string& path, double in_value);
  void SetString(const std::string& path, const std::string& in_value);

  // This may be called before |ReplaceProvider| to copy values currently stored
  // in the old provider. If the method is called after |ReplaceProvider|, then
  // the value is retrieved from |real_settings_provider_| for any |path|.
  void CopyStoredValue(const std::string& path);

  // Write the setting from |path| to local state so that it can be retrieved
  // later on browser test startup by the device settings service.
  void StoreCachedDeviceSetting(const std::string& path);

 private:
  // Helpers used to mock out cros settings.
  std::unique_ptr<ScopedTestDeviceSettingsService>
      test_device_settings_service_;
  std::unique_ptr<ScopedTestCrosSettings> test_cros_settings_;
  std::unique_ptr<CrosSettingsProvider> real_settings_provider_;
  std::unique_ptr<CrosSettingsProvider> stub_settings_provider_;
  StubCrosSettingsProvider* stub_settings_provider_ptr_;

  void Initialize(bool create_settings_service);

  DISALLOW_COPY_AND_ASSIGN(ScopedCrosSettingsTestHelper);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SETTINGS_SCOPED_CROS_SETTINGS_TEST_HELPER_H_
