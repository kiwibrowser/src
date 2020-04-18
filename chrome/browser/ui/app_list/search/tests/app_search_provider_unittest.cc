// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/app_search_provider.h"

#include <stddef.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/simple_test_clock.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/ui/app_list/app_list_test_util.h"
#include "chrome/browser/ui/app_list/arc/arc_app_item.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/app_list/arc/arc_app_test.h"
#include "chrome/browser/ui/app_list/extension_app_model_builder.h"
#include "chrome/browser/ui/app_list/search/chrome_search_result.h"
#include "chrome/browser/ui/app_list/test/fake_app_list_model_updater.h"
#include "chrome/browser/ui/app_list/test/test_app_list_controller_delegate.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/test/base/testing_profile.h"
#include "components/arc/test/fake_app_instance.h"
#include "components/sync/model/string_ordinal.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/uninstall_reason.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/extension_set.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace app_list {
namespace test {

namespace {

constexpr char kGmailQeuery[] = "Gmail";
constexpr char kGmailArcName[] = "Gmail ARC";
constexpr char kGmailExtensionName[] = "Gmail Ext";
constexpr char kGmailArcPackage[] = "com.google.android.gm";
constexpr char kGmailArcActivity[] =
    "com.google.android.gm.ConversationListActivityGmail";
constexpr char kKeyboardShortcutHelperInternalName[] = "Shortcuts";
constexpr char kSettingsInternalName[] = "Settings";

}  // namespace

const base::Time kTestCurrentTime = base::Time::FromInternalValue(100000);

bool MoreRelevant(const ChromeSearchResult* result1,
                  const ChromeSearchResult* result2) {
  return result1->relevance() > result2->relevance();
}

class AppSearchProviderTest : public AppListTestBase {
 public:
  AppSearchProviderTest() {}
  ~AppSearchProviderTest() override {}

  // AppListTestBase overrides:
  void SetUp() override {
    AppListTestBase::SetUp();

    model_updater_ = std::make_unique<FakeAppListModelUpdater>();
    controller_.reset(new ::test::TestAppListControllerDelegate);
  }

  void CreateSearch() {
    clock_.SetNow(kTestCurrentTime);
    app_search_.reset(new AppSearchProvider(profile_.get(), nullptr, &clock_,
                                            model_updater_.get()));
  }

  std::string RunQuery(const std::string& query) {
    app_search_->Start(base::UTF8ToUTF16(query));

    // Sort results by relevance.
    std::vector<ChromeSearchResult*> sorted_results;
    for (const auto& result : app_search_->results())
      sorted_results.emplace_back(result.get());
    std::sort(sorted_results.begin(), sorted_results.end(), &MoreRelevant);

    std::string result_str;
    for (size_t i = 0; i < sorted_results.size(); ++i) {
      if (!result_str.empty())
        result_str += ',';

      result_str += base::UTF16ToUTF8(sorted_results[i]->title());
    }
    return result_str;
  }

  std::string AddArcApp(const std::string& name,
                        const std::string& package,
                        const std::string& activity) {
    arc::mojom::AppInfo app_info;
    app_info.name = name;
    app_info.package_name = package;
    app_info.activity = activity;
    app_info.sticky = false;
    app_info.notifications_enabled = false;
    arc_test_.app_instance()->SendAppAdded(app_info);
    return ArcAppListPrefs::GetAppId(package, activity);
  }

  void AddExtension(const std::string& id, const std::string& name) {
    scoped_refptr<extensions::Extension> extension =
        extensions::ExtensionBuilder()
            .SetManifest(
                extensions::DictionaryBuilder()
                    .Set("name", name)
                    .Set("version", "0.1")
                    .Set("app",
                         extensions::DictionaryBuilder()
                             .Set("urls",
                                  extensions::ListBuilder()
                                      .Append("http://localhost/extensions/"
                                              "hosted_app/main.html")
                                      .Build())
                             .Build())
                    .Set("launch",
                         extensions::DictionaryBuilder()
                             .Set("urls",
                                  extensions::ListBuilder()
                                      .Append("http://localhost/extensions/"
                                              "hosted_app/main.html")
                                      .Build())
                             .Build())
                    .Build())
            .SetID(id)
            .Build();
    service()->AddExtension(extension.get());
  }

  const SearchProvider::Results& results() { return app_search_->results(); }
  ArcAppTest& arc_test() { return arc_test_; }

 private:
  base::SimpleTestClock clock_;
  std::unique_ptr<FakeAppListModelUpdater> model_updater_;
  std::unique_ptr<AppSearchProvider> app_search_;
  std::unique_ptr<::test::TestAppListControllerDelegate> controller_;
  ArcAppTest arc_test_;

  DISALLOW_COPY_AND_ASSIGN(AppSearchProviderTest);
};

TEST_F(AppSearchProviderTest, Basic) {
  arc_test().SetUp(profile());
  arc_test().app_instance()->RefreshAppList();
  std::vector<arc::mojom::AppInfo> arc_apps(arc_test().fake_apps().begin(),
                                            arc_test().fake_apps().begin() + 2);
  arc_test().app_instance()->SendRefreshAppList(arc_apps);

  CreateSearch();

  EXPECT_EQ("", RunQuery("!@#$-,-_"));
  EXPECT_EQ("", RunQuery("unmatched query"));

  // Search for "pa" should return both packaged app. The order is undefined
  // because the test only considers textual relevance and the two apps end
  // up having the same score.
  std::string result = RunQuery("pa");
  EXPECT_TRUE(result == "Packaged App 1,Packaged App 2" ||
              result == "Packaged App 2,Packaged App 1");

  EXPECT_EQ("Packaged App 1", RunQuery("pa1"));
  EXPECT_EQ("Packaged App 2", RunQuery("pa2"));
  EXPECT_EQ("Packaged App 1", RunQuery("papp1"));
  EXPECT_EQ("Hosted App", RunQuery("host"));

  result = RunQuery("fake");
  EXPECT_TRUE(result == "Fake App 0,Fake App 1" ||
              result == "Fake App 1,Fake App 0");
  result = RunQuery("app1");
  EXPECT_TRUE(result == "Packaged App 1,Fake App 1" ||
              result == "Fake App 1,Packaged App 1");
}

TEST_F(AppSearchProviderTest, DisableAndEnable) {
  CreateSearch();

  EXPECT_EQ("Hosted App", RunQuery("host"));

  service_->DisableExtension(kHostedAppId,
                             extensions::disable_reason::DISABLE_USER_ACTION);
  EXPECT_EQ("Hosted App", RunQuery("host"));

  service_->EnableExtension(kHostedAppId);
  EXPECT_EQ("Hosted App", RunQuery("host"));
}

TEST_F(AppSearchProviderTest, UninstallExtension) {
  CreateSearch();

  EXPECT_EQ("Packaged App 1", RunQuery("pa1"));
  EXPECT_FALSE(results().empty());
  service_->UninstallExtension(kPackagedApp1Id,
                               extensions::UNINSTALL_REASON_FOR_TESTING, NULL);

  // Allow async AppSearchProvider::UpdateResults to run.
  base::RunLoop().RunUntilIdle();

  // Uninstalling an app should update the result list without needing to start
  // a new search.
  EXPECT_TRUE(results().empty());

  // Rerunning the query also should return no results.
  EXPECT_EQ("", RunQuery("pa1"));

  // Let uninstall code to clean up.
  base::RunLoop().RunUntilIdle();
}

TEST_F(AppSearchProviderTest, InstallUninstallArc) {
  arc_test().SetUp(profile());
  std::vector<arc::mojom::AppInfo> arc_apps;
  arc_test().app_instance()->RefreshAppList();
  arc_test().app_instance()->SendRefreshAppList(arc_apps);

  CreateSearch();

  EXPECT_TRUE(results().empty());
  EXPECT_EQ("", RunQuery("fapp0"));

  arc_apps.push_back(arc_test().fake_apps()[0]);
  arc_test().app_instance()->RefreshAppList();
  arc_test().app_instance()->SendRefreshAppList(arc_apps);

  // Allow async AppSearchProvider::UpdateResults to run.
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ("Fake App 0", RunQuery("fapp0"));
  EXPECT_FALSE(results().empty());

  arc_apps.clear();
  arc_test().app_instance()->RefreshAppList();
  arc_test().app_instance()->SendRefreshAppList(arc_apps);

  // Allow async AppSearchProvider::UpdateResults to run.
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(results().empty());
  EXPECT_EQ("", RunQuery("fapp0"));

  // Let uninstall code to clean up.
  base::RunLoop().RunUntilIdle();
}

TEST_F(AppSearchProviderTest, FetchRecommendations) {
  CreateSearch();

  extensions::ExtensionPrefs* prefs =
      extensions::ExtensionPrefs::Get(profile_.get());

  prefs->SetLastLaunchTime(kHostedAppId, base::Time::FromInternalValue(20));
  prefs->SetLastLaunchTime(kPackagedApp1Id, base::Time::FromInternalValue(10));
  prefs->SetLastLaunchTime(kPackagedApp2Id, base::Time::FromInternalValue(5));
  EXPECT_EQ("Hosted App,Packaged App 1,Packaged App 2,Settings", RunQuery(""));

  prefs->SetLastLaunchTime(kHostedAppId, base::Time::FromInternalValue(5));
  prefs->SetLastLaunchTime(kPackagedApp1Id, base::Time::FromInternalValue(10));
  prefs->SetLastLaunchTime(kPackagedApp2Id, base::Time::FromInternalValue(20));
  EXPECT_EQ("Packaged App 2,Packaged App 1,Hosted App,Settings", RunQuery(""));

  // Times in the future should just be handled as highest priority.
  prefs->SetLastLaunchTime(kHostedAppId,
                           kTestCurrentTime + base::TimeDelta::FromSeconds(5));
  prefs->SetLastLaunchTime(kPackagedApp1Id, base::Time::FromInternalValue(10));
  prefs->SetLastLaunchTime(kPackagedApp2Id, base::Time::FromInternalValue(5));
  EXPECT_EQ("Hosted App,Packaged App 1,Packaged App 2,Settings", RunQuery(""));
}

TEST_F(AppSearchProviderTest, FetchUnlaunchedRecommendations) {
  CreateSearch();

  extensions::ExtensionPrefs* prefs =
      extensions::ExtensionPrefs::Get(profile_.get());

  // The order of unlaunched recommendations should be based on the install time
  // order.
  prefs->SetLastLaunchTime(kHostedAppId, base::Time::Now());
  prefs->SetLastLaunchTime(kPackagedApp1Id, base::Time::FromInternalValue(0));
  prefs->SetLastLaunchTime(kPackagedApp2Id, base::Time::FromInternalValue(0));
  EXPECT_EQ("Hosted App,Packaged App 1,Packaged App 2,Settings", RunQuery(""));
}

TEST_F(AppSearchProviderTest, FilterDuplicate) {
  arc_test().SetUp(profile());

  extensions::ExtensionPrefs* extension_prefs =
      extensions::ExtensionPrefs::Get(profile_.get());
  ASSERT_TRUE(extension_prefs);

  AddExtension(extension_misc::kGmailAppId, kGmailExtensionName);
  const std::string arc_gmail_app_id =
      AddArcApp(kGmailArcName, kGmailArcPackage, kGmailArcActivity);
  arc_test().arc_app_list_prefs()->SetLastLaunchTime(arc_gmail_app_id);

  std::unique_ptr<ArcAppListPrefs::AppInfo> arc_gmail_app_info =
      arc_test().arc_app_list_prefs()->GetApp(arc_gmail_app_id);
  ASSERT_TRUE(arc_gmail_app_info);

  EXPECT_FALSE(arc_gmail_app_info->last_launch_time.is_null());
  EXPECT_FALSE(arc_gmail_app_info->install_time.is_null());

  extension_prefs->SetLastLaunchTime(
      extension_misc::kGmailAppId,
      arc_gmail_app_info->last_launch_time - base::TimeDelta::FromSeconds(1));

  CreateSearch();
  EXPECT_EQ(kGmailArcName, RunQuery(kGmailQeuery));

  extension_prefs->SetLastLaunchTime(
      extension_misc::kGmailAppId,
      arc_gmail_app_info->last_launch_time + base::TimeDelta::FromSeconds(1));

  CreateSearch();
  EXPECT_EQ(kGmailExtensionName, RunQuery(kGmailQeuery));
}

TEST_F(AppSearchProviderTest, FetchInternalApp) {
  CreateSearch();

  // Search Keyboard Shortcut Helper.
  EXPECT_EQ(kKeyboardShortcutHelperInternalName, RunQuery("Keyboard"));
  EXPECT_EQ(kKeyboardShortcutHelperInternalName, RunQuery("Shortcut"));
  EXPECT_EQ(kKeyboardShortcutHelperInternalName, RunQuery("Helper"));

  // Search Settings.
  EXPECT_EQ(kSettingsInternalName, RunQuery("Settings"));
  EXPECT_EQ(kSettingsInternalName, RunQuery("Set"));
}

}  // namespace test
}  // namespace app_list
