// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/ownership/fake_owner_settings_service.h"

#include "base/logging.h"
#include "chrome/browser/chromeos/settings/stub_cros_settings_provider.h"
#include "components/ownership/mock_owner_key_util.h"

namespace chromeos {

FakeOwnerSettingsService::FakeOwnerSettingsService(Profile* profile)
    : OwnerSettingsServiceChromeOS(nullptr,
                                   profile,
                                   new ownership::MockOwnerKeyUtil()),
      settings_provider_(nullptr) {
}

FakeOwnerSettingsService::FakeOwnerSettingsService(
    Profile* profile,
    const scoped_refptr<ownership::OwnerKeyUtil>& owner_key_util,
    StubCrosSettingsProvider* provider)
    : OwnerSettingsServiceChromeOS(nullptr, profile, owner_key_util),
      set_management_settings_result_(true),
      settings_provider_(provider) {
}

FakeOwnerSettingsService::~FakeOwnerSettingsService() {
}

bool FakeOwnerSettingsService::Set(const std::string& setting,
                                   const base::Value& value) {
  CHECK(settings_provider_);
  settings_provider_->Set(setting, value);
  return true;
}

void FakeOwnerSettingsService::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!ignore_profile_creation_notifications_)
    OwnerSettingsServiceChromeOS::Observe(type, source, details);
}

}  // namespace chromeos
