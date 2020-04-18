// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/previews/previews_infobar_delegate.h"

#include <map>
#include <memory>
#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop_current.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/metrics/field_trial_params.h"
#include "base/optional.h"
#include "base/run_loop.h"
#include "base/strings/string16.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/android/android_theme_resources.h"
#include "chrome/browser/infobars/mock_infobar_service.h"
#include "chrome/browser/net/spdyproxy/data_reduction_proxy_chrome_settings.h"
#include "chrome/browser/net/spdyproxy/data_reduction_proxy_chrome_settings_factory.h"
#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"
#include "chrome/browser/page_load_metrics/page_load_tracker.h"
#include "chrome/browser/previews/previews_infobar_tab_helper.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_config_test_utils.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_test_utils.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_pref_names.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_delegate.h"
#include "components/network_time/network_time_test_utils.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/previews/content/previews_io_data.h"
#include "components/previews/content/previews_ui_service.h"
#include "components/previews/core/previews_experiments.h"
#include "components/previews/core/previews_features.h"
#include "components/previews/core/previews_logger.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/referrer.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

namespace {

const char kTestUrl[] = "http://www.test.com/";

// Key of the UMA Previews.InfoBarAction.LoFi histogram.
const char kUMAPreviewsInfoBarActionLoFi[] = "Previews.InfoBarAction.LoFi";

// Key of the UMA Previews.InfoBarAction.Offline histogram.
const char kUMAPreviewsInfoBarActionOffline[] =
    "Previews.InfoBarAction.Offline";

// Key of the UMA Previews.InfoBarAction.LitePage histogram.
const char kUMAPreviewsInfoBarActionLitePage[] =
    "Previews.InfoBarAction.LitePage";

// Key of the UMA Previews.InfoBarTimestamp histogram.
const char kUMAPreviewsInfoBarTimestamp[] = "Previews.InfoBarTimestamp";

// Dummy method for creating TestPreviewsUIService.
bool IsPreviewsEnabled(previews::PreviewsType type) {
  return true;
}

class TestPreviewsWebContentsObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<TestPreviewsWebContentsObserver> {
 public:
  explicit TestPreviewsWebContentsObserver(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents),
        last_navigation_reload_type_(content::ReloadType::NONE) {}
  ~TestPreviewsWebContentsObserver() override {}

  content::ReloadType last_navigation_reload_type() {
    return last_navigation_reload_type_;
  }

  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override {
    last_navigation_reload_type_ = navigation_handle->GetReloadType();
  }

 private:
  content::ReloadType last_navigation_reload_type_;
};

class TestOptOutObserver : public page_load_metrics::PageLoadMetricsObserver {
 public:
  explicit TestOptOutObserver(const base::Callback<void()>& callback)
      : callback_(callback) {}
  ~TestOptOutObserver() override {}

  void OnEventOccurred(const void* const event_key) override {
    if (PreviewsInfoBarDelegate::OptOutEventKey() == event_key)
      callback_.Run();
  }

  base::Callback<void()> callback_;
};

class TestPreviewsLogger : public previews::PreviewsLogger {
 public:
  TestPreviewsLogger() {}
  ~TestPreviewsLogger() override {}

  // previews::PreviewsLogger:
  void LogMessage(const std::string& event_type,
                  const std::string& event_description,
                  const GURL& url,
                  base::Time time,
                  uint64_t page_id) override {
    event_type_ = event_type;
    event_description_ = event_description;
  }

  // Exposed passed in params of LogMessage for testing.
  std::string event_type() const { return event_type_; }
  std::string event_description() const { return event_description_; }

 private:
  // Passed in parameters of LogMessage.
  std::string event_type_;
  std::string event_description_;
};

}  // namespace

DEFINE_WEB_CONTENTS_USER_DATA_KEY(TestPreviewsWebContentsObserver);

class PreviewsInfoBarDelegateUnitTest
    : public page_load_metrics::PageLoadMetricsObserverTestHarness {
 protected:
  PreviewsInfoBarDelegateUnitTest()
      : opt_out_called_(false),
        field_trial_list_(new base::FieldTrialList(nullptr)),
        tester_(new base::HistogramTester()) {}

  void SetUp() override {
    PageLoadMetricsObserverTestHarness::SetUp();
    MockInfoBarService::CreateForWebContents(web_contents());
    PreviewsInfoBarTabHelper::CreateForWebContents(web_contents());
    TestPreviewsWebContentsObserver::CreateForWebContents(web_contents());

    drp_test_context_ =
        data_reduction_proxy::DataReductionProxyTestContext::Builder()
            .WithMockConfig()
            .SkipSettingsInitialization()
            .Build();

    auto* data_reduction_proxy_settings =
        DataReductionProxyChromeSettingsFactory::GetForBrowserContext(
            web_contents()->GetBrowserContext());

    PrefRegistrySimple* registry =
        drp_test_context_->pref_service()->registry();
    registry->RegisterDictionaryPref(proxy_config::prefs::kProxy);
    data_reduction_proxy_settings
        ->set_data_reduction_proxy_enabled_pref_name_for_test(
            drp_test_context_->GetDataReductionProxyEnabledPrefName());
    data_reduction_proxy_settings->InitDataReductionProxySettings(
        drp_test_context_->io_data(), drp_test_context_->pref_service(),
        drp_test_context_->request_context_getter(),
        base::WrapUnique(new data_reduction_proxy::DataStore()),
        base::ThreadTaskRunnerHandle::Get(),
        base::ThreadTaskRunnerHandle::Get());

    TestingBrowserProcess::GetGlobal()->SetLocalState(
        drp_test_context_->pref_service());
    network_time::NetworkTimeTracker::RegisterPrefs(registry);

    std::unique_ptr<TestPreviewsLogger> previews_logger =
        std::make_unique<TestPreviewsLogger>();
    previews_logger_ = previews_logger.get();
    previews_io_data_ = std::make_unique<previews::PreviewsIOData>(
        base::MessageLoopCurrent::Get()->task_runner(),
        base::MessageLoopCurrent::Get()->task_runner());
    previews_ui_service_ = std::make_unique<previews::PreviewsUIService>(
        previews_io_data_.get(), base::MessageLoopCurrent::Get()->task_runner(),
        nullptr /* previews_opt_out_store */, nullptr /* previews_opt_guide */,
        base::Bind(&IsPreviewsEnabled), std::move(previews_logger));
    base::RunLoop().RunUntilIdle();
  }

  void TearDown() override {
    drp_test_context_->DestroySettings();
    ChromeRenderViewHostTestHarness::TearDown();
    TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);
  }

  PreviewsInfoBarDelegate* CreateInfoBar(previews::PreviewsType type,
                                         base::Time previews_freshness,
                                         bool is_data_saver_user,
                                         bool is_reload) {
    PreviewsInfoBarDelegate::Create(
        web_contents(), type, previews_freshness, is_data_saver_user, is_reload,
        base::Bind(&PreviewsInfoBarDelegateUnitTest::OnDismissPreviewsInfobar,
                   base::Unretained(this)),
        previews_ui_service_.get());

    EXPECT_EQ(1U, infobar_service()->infobar_count());

    return static_cast<PreviewsInfoBarDelegate*>(
        infobar_service()->infobar_at(0)->delegate());
  }

  void EnableStalePreviewsTimestamp(
      const std::map<std::string, std::string>& variation_params) {
    field_trial_list_.reset();
    field_trial_list_.reset(new base::FieldTrialList(nullptr));
    base::FieldTrialParamAssociator::GetInstance()->ClearAllParamsForTesting();

    const std::string kTrialName = "TrialName";
    const std::string kGroupName = "GroupName";

    base::AssociateFieldTrialParams(kTrialName, kGroupName, variation_params);
    base::FieldTrial* field_trial =
        base::FieldTrialList::CreateFieldTrial(kTrialName, kGroupName);

    std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);
    feature_list->RegisterFieldTrialOverride(
        previews::features::kStalePreviewsTimestamp.name,
        base::FeatureList::OVERRIDE_ENABLE_FEATURE, field_trial);
    scoped_feature_list_.InitWithFeatureList(std::move(feature_list));
  }

  void TestStalePreviews(
      int staleness_in_minutes,
      bool is_reload,
      base::string16 expected_timestamp,
      PreviewsInfoBarDelegate::PreviewsInfoBarTimestamp expected_bucket) {
    PreviewsInfoBarDelegate* infobar = CreateInfoBar(
        previews::PreviewsType::LITE_PAGE,
        base::Time::Now() - base::TimeDelta::FromMinutes(staleness_in_minutes),
        true /* is_data_saver_user */, is_reload);
    EXPECT_EQ(expected_timestamp, infobar->GetTimestampText());
    tester_->ExpectBucketCount(kUMAPreviewsInfoBarTimestamp, expected_bucket,
                               1);
    // Dismiss the infobar.
    infobar_service()->RemoveAllInfoBars(false);
    PreviewsInfoBarTabHelper::FromWebContents(web_contents())
        ->set_displayed_preview_infobar(false);
  }

  void OnDismissPreviewsInfobar(bool user_opt_out) {
    user_opt_out_ = user_opt_out;
  }

  InfoBarService* infobar_service() {
    return InfoBarService::FromWebContents(web_contents());
  }

  // Expose previews_logger_ raw pointer to test results.
  TestPreviewsLogger* previews_logger() const { return previews_logger_; }

  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    tracker->AddObserver(std::make_unique<TestOptOutObserver>(base::Bind(
        &PreviewsInfoBarDelegateUnitTest::OptOut, base::Unretained(this))));
  }

  void OptOut() { opt_out_called_ = true; }

  bool opt_out_called_;

  std::unique_ptr<data_reduction_proxy::DataReductionProxyTestContext>
      drp_test_context_;

  base::Optional<bool> user_opt_out_;
  std::unique_ptr<base::FieldTrialList> field_trial_list_;
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<base::HistogramTester> tester_;

  TestPreviewsLogger* previews_logger_;
  std::unique_ptr<previews::PreviewsIOData> previews_io_data_;
  std::unique_ptr<previews::PreviewsUIService> previews_ui_service_;
};

// TODO(crbug/782740): Test temporarily disabled on Windows because it crashes
// on trybots.
#if defined(OS_WIN)
#define DISABLE_ON_WINDOWS(x) DISABLED_##x
#else
#define DISABLE_ON_WINDOWS(x) x
#endif
TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(InfobarTestNavigationDismissal)) {
  CreateInfoBar(previews::PreviewsType::LOFI, base::Time(),
                true /* is_data_saver_user */, false /* is_reload */);

  // Try showing a second infobar. Another should not be shown since the page
  // has not navigated.
  PreviewsInfoBarDelegate::Create(
      web_contents(), previews::PreviewsType::LOFI,
      base::Time() /* previews_freshness */, true /* is_data_saver_user */,
      false /* is_reload */,
      PreviewsInfoBarDelegate::OnDismissPreviewsInfobarCallback(),
      previews_ui_service_.get());
  EXPECT_EQ(1U, infobar_service()->infobar_count());

  // Navigate and make sure the infobar is dismissed.
  NavigateAndCommit(GURL(kTestUrl));
  EXPECT_EQ(0U, infobar_service()->infobar_count());
  EXPECT_FALSE(user_opt_out_.value());

  tester_->ExpectBucketCount(
      kUMAPreviewsInfoBarActionLoFi,
      PreviewsInfoBarDelegate::INFOBAR_DISMISSED_BY_NAVIGATION, 1);
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(InfobarTestReloadDismissal)) {
  // Navigate to test URL, so we can reload later.
  NavigateAndCommit(GURL(kTestUrl));

  CreateInfoBar(previews::PreviewsType::LOFI, base::Time(),
                true /* is_data_saver_user */, false /* is_reload */);

  // Try showing a second infobar. Another should not be shown since the page
  // has not navigated.
  PreviewsInfoBarDelegate::Create(
      web_contents(), previews::PreviewsType::LOFI,
      base::Time() /* previews_freshness */, true /* is_data_saver_user */,
      false /* is_reload */,
      PreviewsInfoBarDelegate::OnDismissPreviewsInfobarCallback(),
      previews_ui_service_.get());
  EXPECT_EQ(1U, infobar_service()->infobar_count());

  // Navigate to test URL as a reload to dismiss the infobar.
  controller().LoadURL(GURL(kTestUrl), content::Referrer(),
                       ui::PAGE_TRANSITION_RELOAD, std::string());
  content::WebContentsTester::For(web_contents())->CommitPendingNavigation();

  EXPECT_EQ(0U, infobar_service()->infobar_count());
  EXPECT_FALSE(user_opt_out_.value());

  tester_->ExpectBucketCount(
      kUMAPreviewsInfoBarActionLoFi,
      PreviewsInfoBarDelegate::INFOBAR_DISMISSED_BY_RELOAD, 1);

  EXPECT_FALSE(opt_out_called_);
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(InfobarTestUserDismissal)) {
  ConfirmInfoBarDelegate* infobar =
      CreateInfoBar(previews::PreviewsType::LOFI, base::Time(),
                    true /* is_data_saver_user */, false /* is_reload */);

  // Simulate dismissing the infobar.
  infobar->InfoBarDismissed();
  infobar_service()->infobar_at(0)->RemoveSelf();
  EXPECT_EQ(0U, infobar_service()->infobar_count());

  tester_->ExpectBucketCount(kUMAPreviewsInfoBarActionLoFi,
                             PreviewsInfoBarDelegate::INFOBAR_DISMISSED_BY_USER,
                             1);
  EXPECT_FALSE(user_opt_out_.value());
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(InfobarTestTabClosedDismissal)) {
  CreateInfoBar(previews::PreviewsType::LOFI, base::Time(),
                true /* is_data_saver_user */, false /* is_reload */);

  // Delete the infobar without any other infobar actions.
  infobar_service()->infobar_at(0)->RemoveSelf();
  EXPECT_EQ(0U, infobar_service()->infobar_count());

  tester_->ExpectBucketCount(
      kUMAPreviewsInfoBarActionLoFi,
      PreviewsInfoBarDelegate::INFOBAR_DISMISSED_BY_TAB_CLOSURE, 1);
  EXPECT_FALSE(user_opt_out_.value());
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(InfobarTestClickLinkLoFi)) {
  NavigateAndCommit(GURL(kTestUrl));
  const struct {
    bool using_previews_blacklist;
  } tests[] = {
      {true}, {false},
  };
  for (const auto test : tests) {
    opt_out_called_ = false;
    tester_.reset(new base::HistogramTester());
    field_trial_list_.reset();
    field_trial_list_.reset(new base::FieldTrialList(nullptr));
    if (test.using_previews_blacklist) {
      base::FieldTrialList::CreateFieldTrial(
          "DataReductionProxyPreviewsBlackListTransition", "Enabled_");
    }

    // Call Reload and CommitPendingNavigation to force DidFinishNavigation.
    web_contents()->GetController().Reload(content::ReloadType::NORMAL, true);
    content::WebContentsTester::For(web_contents())->CommitPendingNavigation();

    ConfirmInfoBarDelegate* infobar =
        CreateInfoBar(previews::PreviewsType::LOFI, base::Time(),
                      true /* is_data_saver_user */, false /* is_reload */);

    // Simulate clicking the infobar link.
    if (infobar->LinkClicked(WindowOpenDisposition::CURRENT_TAB))
      infobar_service()->infobar_at(0)->RemoveSelf();
    EXPECT_EQ(0U, infobar_service()->infobar_count());

    tester_->ExpectBucketCount(
        kUMAPreviewsInfoBarActionLoFi,
        PreviewsInfoBarDelegate::INFOBAR_LOAD_ORIGINAL_CLICKED, 1);
    EXPECT_TRUE(user_opt_out_.value());

    EXPECT_TRUE(opt_out_called_);
  }
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(InfobarTestClickLinkLitePage)) {
  NavigateAndCommit(GURL(kTestUrl));
  ConfirmInfoBarDelegate* infobar =
      CreateInfoBar(previews::PreviewsType::LITE_PAGE, base::Time(),
                    true /* is_data_saver_user */, false /* is_reload */);

  // Simulate clicking the infobar link.
  if (infobar->LinkClicked(WindowOpenDisposition::CURRENT_TAB))
    infobar_service()->infobar_at(0)->RemoveSelf();
  EXPECT_EQ(0U, infobar_service()->infobar_count());

  tester_->ExpectBucketCount(
      kUMAPreviewsInfoBarActionLitePage,
      PreviewsInfoBarDelegate::INFOBAR_LOAD_ORIGINAL_CLICKED, 1);

  content::WebContentsTester::For(web_contents())->CommitPendingNavigation();

  EXPECT_EQ(content::ReloadType::DISABLE_PREVIEWS,
            TestPreviewsWebContentsObserver::FromWebContents(web_contents())
                ->last_navigation_reload_type());

  EXPECT_TRUE(opt_out_called_);
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(InfobarTestShownOncePerNavigation)) {
  ConfirmInfoBarDelegate* infobar =
      CreateInfoBar(previews::PreviewsType::LOFI, base::Time(),
                    true /* is_data_saver_user */, false /* is_reload */);

  // Simulate dismissing the infobar.
  infobar->InfoBarDismissed();
  infobar_service()->infobar_at(0)->RemoveSelf();
  EXPECT_EQ(0U, infobar_service()->infobar_count());

  PreviewsInfoBarDelegate::Create(
      web_contents(), previews::PreviewsType::LOFI,
      base::Time() /* previews_freshness */, true /* is_data_saver_user */,
      false /* is_reload */,
      PreviewsInfoBarDelegate::OnDismissPreviewsInfobarCallback(),
      previews_ui_service_.get());

  // Infobar should not be shown again since a navigation hasn't happened.
  EXPECT_EQ(0U, infobar_service()->infobar_count());

  // Navigate and show infobar again.
  NavigateAndCommit(GURL(kTestUrl));
  CreateInfoBar(previews::PreviewsType::LOFI, base::Time(),
                true /* is_data_saver_user */, false /* is_reload */);
}

TEST_F(PreviewsInfoBarDelegateUnitTest, DISABLE_ON_WINDOWS(LoFiInfobarTest)) {
  ConfirmInfoBarDelegate* infobar =
      CreateInfoBar(previews::PreviewsType::LOFI, base::Time(),
                    true /* is_data_saver_user */, false /* is_reload */);

  tester_->ExpectUniqueSample(kUMAPreviewsInfoBarActionLoFi,
                              PreviewsInfoBarDelegate::INFOBAR_SHOWN, 1);

  ASSERT_TRUE(infobar);
  ASSERT_EQ(l10n_util::GetStringUTF16(IDS_PREVIEWS_INFOBAR_SAVED_DATA_TITLE),
            infobar->GetMessageText());
  ASSERT_EQ(l10n_util::GetStringUTF16(IDS_PREVIEWS_INFOBAR_LINK),
            infobar->GetLinkText());
#if defined(OS_ANDROID)
  ASSERT_EQ(IDR_ANDROID_INFOBAR_PREVIEWS, infobar->GetIconId());
#else
  ASSERT_EQ(PreviewsInfoBarDelegate::kNoIconID, infobar->GetIconId());
#endif
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(PreviewInfobarTest)) {
  PreviewsInfoBarDelegate* infobar =
      CreateInfoBar(previews::PreviewsType::LITE_PAGE, base::Time(),
                    true /* is_data_saver_user */, false /* is_reload */);

  tester_->ExpectUniqueSample(kUMAPreviewsInfoBarActionLitePage,
                              PreviewsInfoBarDelegate::INFOBAR_SHOWN, 1);

  // Check the strings.
  ASSERT_TRUE(infobar);
  ASSERT_EQ(l10n_util::GetStringUTF16(IDS_PREVIEWS_INFOBAR_SAVED_DATA_TITLE),
            infobar->GetMessageText());
  ASSERT_EQ(l10n_util::GetStringUTF16(IDS_PREVIEWS_INFOBAR_LINK),
            infobar->GetLinkText());
  ASSERT_EQ(base::string16(), infobar->GetTimestampText());
#if defined(OS_ANDROID)
  ASSERT_EQ(IDR_ANDROID_INFOBAR_PREVIEWS, infobar->GetIconId());
#else
  ASSERT_EQ(PreviewsInfoBarDelegate::kNoIconID, infobar->GetIconId());
#endif
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(OfflineInfobarNonDataSaverUserTest)) {
  PreviewsInfoBarDelegate* infobar =
      CreateInfoBar(previews::PreviewsType::OFFLINE, base::Time(),
                    false /* is_data_saver_user */, false /* is_reload */);

  tester_->ExpectUniqueSample(kUMAPreviewsInfoBarActionOffline,
                              PreviewsInfoBarDelegate::INFOBAR_SHOWN, 1);

  // Check the strings.
  ASSERT_TRUE(infobar);
  ASSERT_EQ(l10n_util::GetStringUTF16(IDS_PREVIEWS_INFOBAR_FASTER_PAGE_TITLE),
            infobar->GetMessageText());
  ASSERT_EQ(l10n_util::GetStringUTF16(IDS_PREVIEWS_INFOBAR_LINK),
            infobar->GetLinkText());
  ASSERT_EQ(base::string16(), infobar->GetTimestampText());
#if defined(OS_ANDROID)
  ASSERT_EQ(IDR_ANDROID_INFOBAR_PREVIEWS, infobar->GetIconId());
#else
  ASSERT_EQ(PreviewsInfoBarDelegate::kNoIconID, infobar->GetIconId());
#endif
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(OfflineInfobarDataSaverUserTest)) {
  PreviewsInfoBarDelegate* infobar =
      CreateInfoBar(previews::PreviewsType::OFFLINE, base::Time(),
                    true /* is_data_saver_user */, false /* is_reload */);

  tester_->ExpectUniqueSample(kUMAPreviewsInfoBarActionOffline,
                              PreviewsInfoBarDelegate::INFOBAR_SHOWN, 1);

  // Check the strings.
  ASSERT_TRUE(infobar);
  ASSERT_EQ(l10n_util::GetStringUTF16(IDS_PREVIEWS_INFOBAR_SAVED_DATA_TITLE),
            infobar->GetMessageText());
  ASSERT_EQ(l10n_util::GetStringUTF16(IDS_PREVIEWS_INFOBAR_LINK),
            infobar->GetLinkText());
  ASSERT_EQ(base::string16(), infobar->GetTimestampText());
#if defined(OS_ANDROID)
  ASSERT_EQ(IDR_ANDROID_INFOBAR_PREVIEWS, infobar->GetIconId());
#else
  ASSERT_EQ(PreviewsInfoBarDelegate::kNoIconID, infobar->GetIconId());
#endif
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(OfflineInfobarDisablesLoFi)) {
  NavigateAndCommit(GURL(kTestUrl));

  ConfirmInfoBarDelegate* infobar =
      CreateInfoBar(previews::PreviewsType::OFFLINE, base::Time(),
                    true /* is_data_saver_user */, false /* is_reload */);

  tester_->ExpectUniqueSample(kUMAPreviewsInfoBarActionOffline,
                              PreviewsInfoBarDelegate::INFOBAR_SHOWN, 1);

  // Simulate clicking the infobar link.
  if (infobar->LinkClicked(WindowOpenDisposition::CURRENT_TAB))
    infobar_service()->infobar_at(0)->RemoveSelf();
  EXPECT_EQ(0U, infobar_service()->infobar_count());

  content::WebContentsTester::For(web_contents())->CommitPendingNavigation();

  EXPECT_EQ(content::ReloadType::DISABLE_PREVIEWS,
            TestPreviewsWebContentsObserver::FromWebContents(web_contents())
                ->last_navigation_reload_type());

  EXPECT_TRUE(opt_out_called_);
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(PreviewInfobarTimestampMinutesTest)) {
  // Use default params.
  std::map<std::string, std::string> variation_params;
  EnableStalePreviewsTimestamp(variation_params);
  int staleness_in_minutes = 5;

  TestStalePreviews(
      staleness_in_minutes, false /* is_reload */,
      l10n_util::GetStringFUTF16(IDS_PREVIEWS_INFOBAR_TIMESTAMP_MINUTES,
                                 base::IntToString16(staleness_in_minutes)),
      PreviewsInfoBarDelegate::TIMESTAMP_SHOWN);
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(PreviewInfobarTimestampHourTest)) {
  // Use default variation_params.
  std::map<std::string, std::string> variation_params;
  EnableStalePreviewsTimestamp(variation_params);
  int staleness_in_minutes = 65;

  TestStalePreviews(
      staleness_in_minutes, false /* is_reload */,
      l10n_util::GetStringUTF16(IDS_PREVIEWS_INFOBAR_TIMESTAMP_ONE_HOUR),
      PreviewsInfoBarDelegate::TIMESTAMP_SHOWN);
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(PreviewInfobarTimestampHoursTest)) {
  // Use default variation_params.
  std::map<std::string, std::string> variation_params;
  EnableStalePreviewsTimestamp(variation_params);
  int staleness_in_hours = 2;

  TestStalePreviews(
      staleness_in_hours * 60, false /* is_reload */,
      l10n_util::GetStringFUTF16(IDS_PREVIEWS_INFOBAR_TIMESTAMP_HOURS,
                                 base::IntToString16(staleness_in_hours)),
      PreviewsInfoBarDelegate::TIMESTAMP_SHOWN);
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(PreviewInfobarTimestampFinchParamsUMA)) {
  std::map<std::string, std::string> variation_params;
  variation_params["min_staleness_in_minutes"] = "1";
  variation_params["max_staleness_in_minutes"] = "5";
  EnableStalePreviewsTimestamp(variation_params);

  TestStalePreviews(
      1, false /* is_reload */,
      l10n_util::GetStringFUTF16(IDS_PREVIEWS_INFOBAR_TIMESTAMP_MINUTES,
                                 base::IntToString16(1)),
      PreviewsInfoBarDelegate::TIMESTAMP_SHOWN);

  TestStalePreviews(
      6, false /* is_reload */, base::string16(),
      PreviewsInfoBarDelegate::TIMESTAMP_NOT_SHOWN_STALENESS_GREATER_THAN_MAX);
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(PreviewInfobarTimestampUMA)) {
  // Use default params.
  std::map<std::string, std::string> variation_params;
  EnableStalePreviewsTimestamp(variation_params);

  TestStalePreviews(
      1, false /* is_reload */, base::string16(),
      PreviewsInfoBarDelegate::TIMESTAMP_NOT_SHOWN_PREVIEW_NOT_STALE);
  TestStalePreviews(
      -1, false /* is_reload */, base::string16(),
      PreviewsInfoBarDelegate::TIMESTAMP_NOT_SHOWN_STALENESS_NEGATIVE);
  TestStalePreviews(
      1441, false /* is_reload */, base::string16(),
      PreviewsInfoBarDelegate::TIMESTAMP_NOT_SHOWN_STALENESS_GREATER_THAN_MAX);
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(PreviewInfobarTimestampReloadTest)) {
  // Use default params.
  std::map<std::string, std::string> variation_params;
  EnableStalePreviewsTimestamp(variation_params);
  int staleness_in_minutes = 5;

  TestStalePreviews(
      staleness_in_minutes, false /* is_reload */,
      l10n_util::GetStringFUTF16(IDS_PREVIEWS_INFOBAR_TIMESTAMP_MINUTES,
                                 base::IntToString16(staleness_in_minutes)),
      PreviewsInfoBarDelegate::TIMESTAMP_SHOWN);

  staleness_in_minutes = 1;
  TestStalePreviews(
      staleness_in_minutes, true /* is_reload */,
      l10n_util::GetStringUTF16(IDS_PREVIEWS_INFOBAR_TIMESTAMP_UPDATED_NOW),
      PreviewsInfoBarDelegate::TIMESTAMP_UPDATED_NOW_SHOWN);
}

TEST_F(PreviewsInfoBarDelegateUnitTest,
       DISABLE_ON_WINDOWS(CreateInfoBarLogPreviewsInfoBarType)) {
  const previews::PreviewsType expected_type = previews::PreviewsType::LOFI;
  const std::string expected_event = "InfoBar";
  const std::string expected_description =
      previews::GetStringNameForType(expected_type) + " InfoBar shown";

  CreateInfoBar(expected_type, base::Time(), false /* is_data_saver_user */,
                false /* is_reload */);
  EXPECT_EQ(expected_event, previews_logger()->event_type());
  EXPECT_EQ(expected_description, previews_logger()->event_description());
}
