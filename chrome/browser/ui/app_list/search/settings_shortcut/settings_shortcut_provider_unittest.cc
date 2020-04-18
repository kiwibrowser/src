// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/macros.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/app_list/app_list_test_util.h"
#include "chrome/browser/ui/app_list/search/chrome_search_result.h"
#include "chrome/browser/ui/app_list/search/settings_shortcut/settings_shortcut_provider.h"

namespace app_list {
namespace test {

namespace {

constexpr char kBluetoothName[] = "Bluetooth";

bool cmp(const ChromeSearchResult* result1, const ChromeSearchResult* result2) {
  return result1->title() < result2->title();
}

}  // namespace

class SettingsShortcutProviderTest : public AppListTestBase {
 public:
  SettingsShortcutProviderTest() = default;
  ~SettingsShortcutProviderTest() override = default;

  // AppListTestBase:
  void SetUp() override {
    AppListTestBase::SetUp();
    provider_ = std::make_unique<SettingsShortcutProvider>(profile());
  }

  void TearDown() override {
    provider_.reset();
    AppListTestBase::TearDown();
  }

 protected:
  std::string RunQuery(const std::string& query) {
    provider_->Start(base::UTF8ToUTF16(query));

    std::vector<ChromeSearchResult*> sorted_results;
    for (const auto& result : provider_->results())
      sorted_results.emplace_back(result.get());
    std::sort(sorted_results.begin(), sorted_results.end(), cmp);

    std::vector<std::string> result_str;
    for (const auto* result : sorted_results)
      result_str.emplace_back(base::UTF16ToUTF8(result->title()));

    return base::JoinString(result_str, ",");
  }

 private:
  std::unique_ptr<SettingsShortcutProvider> provider_;

  DISALLOW_COPY_AND_ASSIGN(SettingsShortcutProviderTest);
};

TEST_F(SettingsShortcutProviderTest, Basic) {
  // Search bluetooth.
  EXPECT_EQ(kBluetoothName, RunQuery("blue"));
  EXPECT_EQ(kBluetoothName, RunQuery("tooth"));
  EXPECT_EQ(kBluetoothName, RunQuery("bluetooth"));
}

}  // namespace test
}  // namespace app_list
