// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PRINTING_EXTERNAL_PRINTERS_PREF_BRIDGE_H_
#define CHROME_BROWSER_CHROMEOS_PRINTING_EXTERNAL_PRINTERS_PREF_BRIDGE_H_

#include <string>

#include "base/macros.h"
#include "components/prefs/pref_change_registrar.h"

class Profile;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace chromeos {

class ExternalPrinters;

// A collection of preference names representing the external printer fields.
struct ExternalPrinterPolicies {
  std::string access_mode;
  std::string blacklist;
  std::string whitelist;
};

// Observe preference changes and propogate changes to ExternalPrinters.
class ExternalPrintersPrefBridge {
 public:
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry,
                                   const ExternalPrinterPolicies& policies);

  ExternalPrintersPrefBridge(const ExternalPrinterPolicies& policies,
                             Profile* profile);

 private:
  // Retrieve initial values for preferences.
  void Initialize();

  // Handle update for the access mode policy.
  void AccessModeUpdated();

  // Handle updates for the blacklist policy.
  void BlacklistUpdated();

  // Handle updates for the whitelist policy.
  void WhitelistUpdated();

  Profile* profile_;
  const ExternalPrinterPolicies policies_;
  PrefChangeRegistrar pref_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(ExternalPrintersPrefBridge);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PRINTING_EXTERNAL_PRINTERS_PREF_BRIDGE_H_
