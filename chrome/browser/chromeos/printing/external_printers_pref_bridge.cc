// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/external_printers_pref_bridge.h"

#include <string>
#include <vector>

#include "base/values.h"
#include "chrome/browser/chromeos/printing/external_printers.h"
#include "chrome/browser/chromeos/printing/external_printers_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/policy/policy_constants.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace chromeos {

namespace {

// Extracts the list of strings named |policy_name| from |prefs| and returns it.
std::vector<std::string> FromPrefs(const PrefService* prefs,
                                   const std::string& policy_name) {
  std::vector<std::string> string_list;
  const base::ListValue* list = prefs->GetList(policy_name);
  for (const base::Value& value : *list) {
    if (value.is_string()) {
      string_list.push_back(value.GetString());
    }
  }

  return string_list;
}

}  // namespace

// static
void ExternalPrintersPrefBridge::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry,
    const ExternalPrinterPolicies& policies) {
  // Default value for access mode is AllAccess.
  registry->RegisterIntegerPref(policies.access_mode,
                                ExternalPrinters::ALL_ACCESS);
  registry->RegisterListPref(policies.blacklist);
  registry->RegisterListPref(policies.whitelist);
}

ExternalPrintersPrefBridge::ExternalPrintersPrefBridge(
    const ExternalPrinterPolicies& policies,
    Profile* profile)
    : profile_(profile), policies_(policies) {
  pref_change_registrar_.Init(profile_->GetPrefs());

  pref_change_registrar_.Add(
      policies_.access_mode,
      base::BindRepeating(&ExternalPrintersPrefBridge::AccessModeUpdated,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      policies_.blacklist,
      base::BindRepeating(&ExternalPrintersPrefBridge::BlacklistUpdated,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      policies_.whitelist,
      base::BindRepeating(&ExternalPrintersPrefBridge::WhitelistUpdated,
                          base::Unretained(this)));
  Initialize();
}

void ExternalPrintersPrefBridge::Initialize() {
  BlacklistUpdated();
  WhitelistUpdated();
  AccessModeUpdated();
}

void ExternalPrintersPrefBridge::AccessModeUpdated() {
  const PrefService* prefs = profile_->GetPrefs();
  ExternalPrinters::AccessMode mode = ExternalPrinters::UNSET;
  int mode_val = prefs->GetInteger(policies_.access_mode);
  if (mode_val >= ExternalPrinters::BLACKLIST_ONLY &&
      mode_val <= ExternalPrinters::ALL_ACCESS) {
    mode = static_cast<ExternalPrinters::AccessMode>(mode_val);
  } else {
    LOG(ERROR) << "Unrecognized access mode";
    return;
  }

  base::WeakPtr<ExternalPrinters> printers =
      ExternalPrintersFactory::Get()->GetForProfile(profile_);
  if (printers)
    printers->SetAccessMode(mode);
}

void ExternalPrintersPrefBridge::BlacklistUpdated() {
  base::WeakPtr<ExternalPrinters> printers =
      ExternalPrintersFactory::Get()->GetForProfile(profile_);
  if (printers)
    printers->SetBlacklist(
        FromPrefs(profile_->GetPrefs(), policies_.blacklist));
}

void ExternalPrintersPrefBridge::WhitelistUpdated() {
  base::WeakPtr<ExternalPrinters> printers =
      ExternalPrintersFactory::Get()->GetForProfile(profile_);
  if (printers)
    printers->SetWhitelist(
        FromPrefs(profile_->GetPrefs(), policies_.whitelist));
}

}  // namespace chromeos
