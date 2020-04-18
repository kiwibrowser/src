// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/local_state/local_state_ui.h"

#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(LocalStateUiTest, FilterPrefs) {
  std::vector<std::string> prefixes = {"foo", "bar", "baz"};

  std::vector<std::string> invalid_pref_keys = {"fo", "ar", "afoo"};
  std::vector<std::string> valid_pref_keys = {"foo", "foom", "bar.stuff"};

  std::vector<std::string> all_pref_keys = invalid_pref_keys;
  all_pref_keys.insert(all_pref_keys.end(), valid_pref_keys.begin(),
                       valid_pref_keys.end());

  base::DictionaryValue prefs;
  for (const std::string& key : all_pref_keys) {
    prefs.SetString(key, key + "_value");
  }

  internal::FilterPrefs(prefixes, &prefs);

  for (const std::string& invalid_key : invalid_pref_keys) {
    std::string value;
    EXPECT_FALSE(prefs.GetString(invalid_key, &value));
  }

  for (const std::string& valid_key : valid_pref_keys) {
    std::string value;
    EXPECT_TRUE(prefs.GetString(valid_key, &value));
    EXPECT_EQ(valid_key + "_value", value);
  }
}
