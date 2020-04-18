// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/safe_browsing_handler.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "content/public/browser/web_ui.h"

namespace settings {

namespace {

base::DictionaryValue GetSberStateDictionaryValue(const PrefService& prefs) {
  base::DictionaryValue dict;
  dict.SetBoolean("enabled", safe_browsing::IsExtendedReportingEnabled(prefs));
  // TODO(crbug.com/813107): SBEROIA policy is being deprecated, revisit this
  // after it is removed.
  dict.SetBoolean("managed",
                  !safe_browsing::IsExtendedReportingOptInAllowed(prefs) ||
                      safe_browsing::IsExtendedReportingPolicyManaged(prefs));
  return dict;
}

}  // namespace

SafeBrowsingHandler::SafeBrowsingHandler(PrefService* prefs) : prefs_(prefs) {}
SafeBrowsingHandler::~SafeBrowsingHandler() {}

void SafeBrowsingHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getSafeBrowsingExtendedReporting",
      base::BindRepeating(
          &SafeBrowsingHandler::HandleGetSafeBrowsingExtendedReporting,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setSafeBrowsingExtendedReportingEnabled",
      base::BindRepeating(
          &SafeBrowsingHandler::HandleSetSafeBrowsingExtendedReportingEnabled,
          base::Unretained(this)));
}

void SafeBrowsingHandler::OnJavascriptAllowed() {
  profile_pref_registrar_.Init(prefs_);
  profile_pref_registrar_.Add(
      prefs::kSafeBrowsingExtendedReportingEnabled,
      base::Bind(&SafeBrowsingHandler::OnPrefChanged, base::Unretained(this)));
  profile_pref_registrar_.Add(
      prefs::kSafeBrowsingScoutReportingEnabled,
      base::Bind(&SafeBrowsingHandler::OnPrefChanged, base::Unretained(this)));
}

void SafeBrowsingHandler::OnJavascriptDisallowed() {
  profile_pref_registrar_.RemoveAll();
}

void SafeBrowsingHandler::HandleGetSafeBrowsingExtendedReporting(
    const base::ListValue* args) {
  AllowJavascript();
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));

  ResolveJavascriptCallback(*callback_id, GetSberStateDictionaryValue(*prefs_));
}

void SafeBrowsingHandler::HandleSetSafeBrowsingExtendedReportingEnabled(
    const base::ListValue* args) {
  bool enabled;
  CHECK(args->GetBoolean(0, &enabled));
  safe_browsing::SetExtendedReportingPrefAndMetric(
      prefs_, enabled, safe_browsing::SBER_OPTIN_SITE_CHROME_SETTINGS);
}

void SafeBrowsingHandler::OnPrefChanged(const std::string& pref_name) {
  DCHECK(pref_name == prefs::kSafeBrowsingExtendedReportingEnabled ||
         pref_name == prefs::kSafeBrowsingScoutReportingEnabled);

  FireWebUIListener("safe-browsing-extended-reporting-change",
                    GetSberStateDictionaryValue(*prefs_));
}

}  // namespace settings
