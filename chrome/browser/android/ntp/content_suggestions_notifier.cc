// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/ntp/content_suggestions_notifier.h"

#include <limits>

#include "chrome/common/pref_names.h"
#include "components/ntp_snippets/features.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/variations/variations_associated_data.h"

using ntp_snippets::kNotificationsFeature;
using ntp_snippets::kNotificationsIgnoredLimitParam;
using ntp_snippets::kNotificationsIgnoredDefaultLimit;

namespace {

// Whether auto opt out is enabled. Note that this does not disable collection
// of data required for auto opt out. Auto opt out is currently disabled,
// because notification settings page is shown when kNotificationsFeature is
// enabled.
const bool kEnableAutoOptOutDefault = false;
const char kEnableAutoOptOutParamName[] = "enable_auto_opt_out";

bool IsAutoOptOutEnabled() {
  return variations::GetVariationParamByFeatureAsBool(
      ntp_snippets::kNotificationsFeature, kEnableAutoOptOutParamName,
      kEnableAutoOptOutDefault);
}

}  // namespace

bool ContentSuggestionsNotifier::ShouldSendNotifications(PrefService* prefs) {
  // Notifications are blocked when the suggested articles list is hidden.
  // The user can hide the list when kArticleSuggestionsExpandableHeader feature
  // is enabled.
  if (!prefs->GetBoolean(ntp_snippets::prefs::kArticlesListVisible)) {
    return false;
  }

  if (!prefs->GetBoolean(prefs::kContentSuggestionsNotificationsEnabled)) {
    return false;
  }

  if (IsAutoOptOutEnabled()) {
    int current =
        prefs->GetInteger(prefs::kContentSuggestionsConsecutiveIgnoredPrefName);
    int limit = variations::GetVariationParamByFeatureAsInt(
        kNotificationsFeature, kNotificationsIgnoredLimitParam,
        kNotificationsIgnoredDefaultLimit);
    if (current >= limit) {
      return false;
    }
  }

  return true;
}

ContentSuggestionsNotifier::ContentSuggestionsNotifier() = default;
ContentSuggestionsNotifier::~ContentSuggestionsNotifier() = default;
