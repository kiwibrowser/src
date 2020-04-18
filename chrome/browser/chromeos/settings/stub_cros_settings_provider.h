// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SETTINGS_STUB_CROS_SETTINGS_PROVIDER_H_
#define CHROME_BROWSER_CHROMEOS_SETTINGS_STUB_CROS_SETTINGS_PROVIDER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "chromeos/settings/cros_settings_provider.h"
#include "components/prefs/pref_value_map.h"

namespace chromeos {

// CrosSettingsProvider implementation that stores settings in memory unsigned.
class StubCrosSettingsProvider : public CrosSettingsProvider {
 public:
  explicit StubCrosSettingsProvider(const NotifyObserversCallback& notify_cb);
  StubCrosSettingsProvider();
  ~StubCrosSettingsProvider() override;

  // CrosSettingsProvider implementation.
  const base::Value* Get(const std::string& path) const override;
  TrustedStatus PrepareTrustedValues(const base::Closure& callback) override;
  bool HandlesSetting(const std::string& path) const override;

  void SetTrustedStatus(TrustedStatus status);
  void SetCurrentUserIsOwner(bool owner);

 private:
  // CrosSettingsProvider implementation:
  void DoSet(const std::string& path, const base::Value& value) override;

  // Initializes settings to their defaults.
  void SetDefaults();

  // In-memory settings storage.
  PrefValueMap values_;

  // Some tests imply that calling Set() as non-owner doesn't change the actual
  // value but still trigger a notification. For such cases, it is possible to
  // emulate this behavior by changing the ownership status to non-owner with
  // |SetCurrentUserIsOwner(false)|.
  bool current_user_is_owner_ = true;

  TrustedStatus trusted_status_ = CrosSettingsProvider::TRUSTED;

  // Pending callbacks to invoke when switching away from TEMPORARILY_UNTRUSTED.
  std::vector<base::Closure> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(StubCrosSettingsProvider);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SETTINGS_STUB_CROS_SETTINGS_PROVIDER_H_
