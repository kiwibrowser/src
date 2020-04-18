// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_PREFERENCES_PREFS_H_
#define CHROME_BROWSER_ANDROID_PREFERENCES_PREFS_H_

#include <cstddef>

#include "build/build_config.h"
#include "chrome/browser/android/contextual_suggestions/contextual_suggestions_prefs.h"
#include "chrome/common/pref_names.h"
#include "components/dom_distiller/core/pref_names.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/payments/core/payment_prefs.h"

// A preference exposed to Java.
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.preferences
enum Pref {
  ALLOW_DELETING_BROWSER_HISTORY,
  CONTEXTUAL_SUGGESTIONS_ENABLED,
  INCOGNITO_MODE_AVAILABILITY,
  NTP_ARTICLES_SECTION_ENABLED,
  NTP_ARTICLES_LIST_VISIBLE,
  READER_FOR_ACCESSIBILITY_ENABLED,
  PROMPT_FOR_DOWNLOAD_ANDROID,
  SHOW_MISSING_SD_CARD_ERROR_ANDROID,
  CAN_MAKE_PAYMENT_ENABLED,
  // PREF_NUM_PREFS must be the last entry.
  PREF_NUM_PREFS
};

// The indices must match value of Pref.
// Remember to update prefs_unittest.cc as well.
const char* const kPrefsExposedToJava[] = {
    prefs::kAllowDeletingBrowserHistory,
    contextual_suggestions::prefs::kContextualSuggestionsEnabled,
    prefs::kIncognitoModeAvailability,
    ntp_snippets::prefs::kEnableSnippets,
    ntp_snippets::prefs::kArticlesListVisible,
    dom_distiller::prefs::kReaderForAccessibility,
    prefs::kPromptForDownloadAndroid,
    prefs::kShowMissingSdCardErrorAndroid,
    payments::kCanMakePaymentEnabled};

#endif  // CHROME_BROWSER_ANDROID_PREFERENCES_PREFS_H_
