// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_OWNERSHIP_FAKE_OWNER_SETTINGS_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_OWNERSHIP_FAKE_OWNER_SETTINGS_SERVICE_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/chromeos/ownership/owner_settings_service_chromeos.h"

class Profile;

namespace ownership {
class OwnerKeyUtil;
}

namespace chromeos {

class StubCrosSettingsProvider;

class FakeOwnerSettingsService : public OwnerSettingsServiceChromeOS {
 public:
  explicit FakeOwnerSettingsService(Profile* profile);
  FakeOwnerSettingsService(
      Profile* profile,
      const scoped_refptr<ownership::OwnerKeyUtil>& owner_key_util,
      StubCrosSettingsProvider* provider);
  ~FakeOwnerSettingsService() override;

  void set_set_management_settings_result(bool success) {
    set_management_settings_result_ = success;
  }

  void set_ignore_profile_creation_notification(bool ignore) {
    ignore_profile_creation_notifications_ = ignore;
  }

  const ManagementSettings& last_settings() const {
    return last_settings_;
  }

  // OwnerSettingsServiceChromeOS:
  bool Set(const std::string& setting, const base::Value& value) override;

  // NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

 private:
  bool set_management_settings_result_ = true;
  ManagementSettings last_settings_;
  StubCrosSettingsProvider* settings_provider_;
  // Creating TestingProfiles after constructing a FakeOwnerSettingsService
  // causes the underlying OwnerSettingsServiceChromeOS::Observe to be called,
  // which can be bad in tests.
  bool ignore_profile_creation_notifications_ = false;

  DISALLOW_COPY_AND_ASSIGN(FakeOwnerSettingsService);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_OWNERSHIP_FAKE_OWNER_SETTINGS_SERVICE_H_
