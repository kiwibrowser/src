// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/print_preview/sticky_settings.h"

#include <memory>

#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace printing {

namespace {

const char kSettingAppState[] = "appState";

}  // namespace

StickySettings::StickySettings() {}

StickySettings::~StickySettings() {}

const std::string* StickySettings::printer_app_state() const {
  return printer_app_state_ ? &printer_app_state_.value() : nullptr;
}

void StickySettings::StoreAppState(const std::string& data) {
  printer_app_state_ = base::make_optional(data);
}

void StickySettings::SaveInPrefs(PrefService* prefs) const {
  auto value = std::make_unique<base::DictionaryValue>();
  if (printer_app_state_)
    value->SetString(kSettingAppState, printer_app_state_.value());
  prefs->Set(prefs::kPrintPreviewStickySettings, *value);
}

void StickySettings::RestoreFromPrefs(PrefService* prefs) {
  const base::DictionaryValue* value =
      prefs->GetDictionary(prefs::kPrintPreviewStickySettings);
  std::string buffer;
  if (value->GetString(kSettingAppState, &buffer))
    StoreAppState(buffer);
}

// static
void StickySettings::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(prefs::kPrintPreviewStickySettings);
}

}  // namespace printing
