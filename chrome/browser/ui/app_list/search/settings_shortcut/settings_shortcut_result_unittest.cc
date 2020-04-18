// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/settings_shortcut/settings_shortcut_result.h"

#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/app_list/app_list_test_util.h"
#include "chrome/browser/ui/app_list/search/chrome_search_result.h"
#include "chrome/browser/ui/app_list/search/settings_shortcut/settings_shortcut_metadata.h"
#include "ui/base/l10n/l10n_util.h"

namespace app_list {
namespace test {

class SettingsShortcutResultTest : public AppListTestBase {
 public:
  SettingsShortcutResultTest() = default;
  ~SettingsShortcutResultTest() override = default;

 protected:
  std::unique_ptr<SettingsShortcutResult> CreateSettingsShortcutResult(
      const SettingsShortcut& settings_shortcut) {
    return std::make_unique<SettingsShortcutResult>(profile(),
                                                    settings_shortcut);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SettingsShortcutResultTest);
};

TEST_F(SettingsShortcutResultTest, Basic) {
  for (const auto& shortcut : GetSettingsShortcutList()) {
    auto result = CreateSettingsShortcutResult(shortcut);
    EXPECT_EQ(shortcut.shortcut_id, result->id());
    EXPECT_EQ(l10n_util::GetStringUTF16(shortcut.name_string_resource_id),
              result->title());
  }
}

}  // namespace test
}  // namespace app_list
