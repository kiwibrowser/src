// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Font Settings Extension API browser tests.

#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace extensions {

// Test of extension API on a standard profile.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, FontSettings) {
  PrefService* prefs = browser()->profile()->GetPrefs();
  prefs->SetString(prefs::kWebKitStandardFontFamilyKorean, "Tahoma");
  prefs->SetString(prefs::kWebKitSansSerifFontFamily, "Arial");
  prefs->SetInteger(prefs::kWebKitDefaultFontSize, 16);
  prefs->SetInteger(prefs::kWebKitDefaultFixedFontSize, 14);
  prefs->SetInteger(prefs::kWebKitMinimumFontSize, 8);

  EXPECT_TRUE(RunExtensionTest("font_settings/standard")) << message_;
}

// Test of extension API in incognito split mode.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, FontSettingsIncognito) {
  PrefService* prefs = browser()->profile()->GetPrefs();
  prefs->SetString(prefs::kWebKitStandardFontFamilyKorean, "Tahoma");
  prefs->SetString(prefs::kWebKitSansSerifFontFamily, "Arial");
  prefs->SetInteger(prefs::kWebKitDefaultFontSize, 16);

  int flags = ExtensionApiTest::kFlagEnableIncognito |
      ExtensionApiTest::kFlagUseIncognito;
  EXPECT_TRUE(RunExtensionSubtest("font_settings/incognito",
                                  "launch.html",
                                  flags));
}

}  // namespace extensions
