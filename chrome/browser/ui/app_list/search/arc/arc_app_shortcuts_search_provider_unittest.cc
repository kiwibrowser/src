// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/arc/arc_app_shortcuts_search_provider.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/arc/icon_decode_request.h"
#include "chrome/browser/ui/app_list/app_list_test_util.h"
#include "chrome/browser/ui/app_list/arc/arc_app_test.h"
#include "chrome/browser/ui/app_list/search/chrome_search_result.h"
#include "chrome/browser/ui/app_list/test/test_app_list_controller_delegate.h"

namespace app_list {

class ArcAppShortcutsSearchProviderTest : public AppListTestBase {
 protected:
  ArcAppShortcutsSearchProviderTest() = default;
  ~ArcAppShortcutsSearchProviderTest() override = default;

  // AppListTestBase:
  void SetUp() override {
    AppListTestBase::SetUp();
    arc_test_.SetUp(profile());
    controller_ = std::make_unique<test::TestAppListControllerDelegate>();
  }

  void TearDown() override {
    controller_.reset();
    arc_test_.TearDown();
    AppListTestBase::TearDown();
  }

  std::unique_ptr<test::TestAppListControllerDelegate> controller_;
  ArcAppTest arc_test_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcAppShortcutsSearchProviderTest);
};

TEST_F(ArcAppShortcutsSearchProviderTest, Basic) {
  constexpr char kQuery[] = "shortlabel";

  auto provider = std::make_unique<ArcAppShortcutsSearchProvider>(
      profile(), controller_.get());
  EXPECT_TRUE(provider->results().empty());
  arc::IconDecodeRequest::DisableSafeDecodingForTesting();

  provider->Start(base::UTF8ToUTF16(kQuery));
  const auto& results = provider->results();
  EXPECT_EQ(3u, results.size());
  // Verify search results.
  for (size_t i = 0; i < results.size(); ++i) {
    EXPECT_EQ(base::StringPrintf("ShortLabel %zu", i),
              base::UTF16ToUTF8(results[i]->title()));
    EXPECT_EQ(ash::SearchResultDisplayType::kTile, results[i]->display_type());
  }
}

}  // namespace app_list
