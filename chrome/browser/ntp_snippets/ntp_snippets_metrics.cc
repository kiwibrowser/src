// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ntp_snippets/ntp_snippets_metrics.h"

#include "base/metrics/histogram_macros.h"

namespace {

const char kContentSuggestionsNotificationImpressions[] =
    "NewTabPage.ContentSuggestions.Notifications.Impressions";
const char kContentSuggestionsNotificationActions[] =
    "NewTabPage.ContentSuggestions.Notifications.Actions";
const char kContentSuggestionsNotificationOptOuts[] =
    "NewTabPage.ContentSuggestions.Notifications.AutoOptOuts";

}  // namespace

void RecordContentSuggestionsNotificationImpression(
    ContentSuggestionsNotificationImpression what) {
  UMA_HISTOGRAM_ENUMERATION(kContentSuggestionsNotificationImpressions, what,
                            ContentSuggestionsNotificationImpression::MAX);
}

void RecordContentSuggestionsNotificationAction(
    ContentSuggestionsNotificationAction what) {
  UMA_HISTOGRAM_ENUMERATION(kContentSuggestionsNotificationActions, what,
                            ContentSuggestionsNotificationAction::MAX);
}

void RecordContentSuggestionsNotificationOptOut(
    ContentSuggestionsNotificationOptOut what) {
  UMA_HISTOGRAM_ENUMERATION(kContentSuggestionsNotificationOptOuts, what,
                            ContentSuggestionsNotificationOptOut::MAX);
}
