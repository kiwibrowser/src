// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/chrome_accept_language_settings.h"

#include "testing/gtest/include/gtest/gtest.h"

// Test the expansion of the Language List.
TEST(ChromeAcceptLanguageSettings, ExpandLanguageList) {
  std::string output = chrome_accept_language_settings::ExpandLanguageList("");
  EXPECT_EQ("", output);

  output = chrome_accept_language_settings::ExpandLanguageList("en-US");
  EXPECT_EQ("en-US,en", output);

  output = chrome_accept_language_settings::ExpandLanguageList("fr");
  EXPECT_EQ("fr", output);

  // The base language is added after all regional codes...
  output = chrome_accept_language_settings::ExpandLanguageList("en-US,en-CA");
  EXPECT_EQ("en-US,en-CA,en", output);

  // ... but before other language families.
  output =
      chrome_accept_language_settings::ExpandLanguageList("en-US,en-CA,fr");
  EXPECT_EQ("en-US,en-CA,en,fr", output);

  output = chrome_accept_language_settings::ExpandLanguageList(
      "en-US,en-CA,fr,en-AU");
  EXPECT_EQ("en-US,en-CA,en,fr,en-AU", output);

  output =
      chrome_accept_language_settings::ExpandLanguageList("en-US,en-CA,fr-CA");
  EXPECT_EQ("en-US,en-CA,en,fr-CA,fr", output);

  // Add a base language even if it's already in the list.
  output = chrome_accept_language_settings::ExpandLanguageList(
      "en-US,fr-CA,it,fr,es-AR,it-IT");
  EXPECT_EQ("en-US,en,fr-CA,fr,it,es-AR,es,it-IT", output);
}
