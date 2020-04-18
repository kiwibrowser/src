// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/preferences/prefs.h"

#include "base/macros.h"
#include "chrome/browser/android/preferences/pref_service_bridge.h"
#include "chrome/common/pref_names.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char* GetPrefName(Pref pref) {
  return PrefServiceBridge::GetPrefNameExposedToJava(pref);
}

TEST(PrefsTest, TestSize) {
  // If this test fails, you most likely forgot to update either Pref enum or
  // |kPrefExposedToJava|.
  EXPECT_EQ(Pref::PREF_NUM_PREFS, arraysize(kPrefsExposedToJava));
}

TEST(PrefsTest, TestIndex) {
  EXPECT_EQ(prefs::kAllowDeletingBrowserHistory,
            GetPrefName(ALLOW_DELETING_BROWSER_HISTORY));
  EXPECT_EQ(prefs::kIncognitoModeAvailability,
            GetPrefName(INCOGNITO_MODE_AVAILABILITY));
  EXPECT_EQ(ntp_snippets::prefs::kEnableSnippets,
            GetPrefName(NTP_ARTICLES_SECTION_ENABLED));
  EXPECT_EQ(ntp_snippets::prefs::kArticlesListVisible,
            GetPrefName(NTP_ARTICLES_LIST_VISIBLE));
  EXPECT_EQ(dom_distiller::prefs::kReaderForAccessibility,
            GetPrefName(READER_FOR_ACCESSIBILITY_ENABLED));
  EXPECT_EQ(payments::kCanMakePaymentEnabled,
            GetPrefName(CAN_MAKE_PAYMENT_ENABLED));
}

}  // namespace
