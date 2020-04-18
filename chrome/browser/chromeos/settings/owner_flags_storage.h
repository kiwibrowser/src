// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SETTINGS_OWNER_FLAGS_STORAGE_H_
#define CHROME_BROWSER_CHROMEOS_SETTINGS_OWNER_FLAGS_STORAGE_H_

#include "base/compiler_specific.h"
#include "components/flags_ui/pref_service_flags_storage.h"

namespace ownership {
class OwnerSettingsService;
}

namespace chromeos {

namespace about_flags {

// Implements the FlagsStorage interface for the owner flags. It inherits from
// PrefServiceFlagsStorage but extends it with storing the flags in the signed
// settings as well which effectively applies them to the login session as well.
class OwnerFlagsStorage : public ::flags_ui::PrefServiceFlagsStorage {
 public:
  OwnerFlagsStorage(PrefService* prefs,
                    ownership::OwnerSettingsService* owner_settings_service);
  ~OwnerFlagsStorage() override;

  bool SetFlags(const std::set<std::string>& flags) override;

 private:
  ownership::OwnerSettingsService* owner_settings_service_;
};

}  // namespace about_flags
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SETTINGS_OWNER_FLAGS_STORAGE_H_
