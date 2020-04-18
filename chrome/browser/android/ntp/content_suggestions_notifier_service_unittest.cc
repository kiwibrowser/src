// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/ntp/content_suggestions_notifier_service.h"

#include <memory>

#include "base/android/application_status_listener.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/simple_test_clock.h"
#include "chrome/browser/android/ntp/content_suggestions_notifier.h"
#include "chrome/common/pref_names.h"
#include "components/ntp_snippets/category_info.h"
#include "components/ntp_snippets/category_rankers/fake_category_ranker.h"
#include "components/ntp_snippets/content_suggestions_service.h"
#include "components/ntp_snippets/logger.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/ntp_snippets/remote/remote_suggestion_builder.h"
#include "components/ntp_snippets/user_classifier.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::android::APPLICATION_STATE_HAS_PAUSED_ACTIVITIES;
using base::android::ApplicationState;
using ntp_snippets::Category;
using ntp_snippets::CategoryInfo;
using ntp_snippets::CategoryStatus;
using ntp_snippets::ContentSuggestion;
using ntp_snippets::ContentSuggestionsAdditionalAction;
using ntp_snippets::ContentSuggestionsCardLayout;
using ntp_snippets::ContentSuggestionsProvider;
using ntp_snippets::ContentSuggestionsService;
using ntp_snippets::DismissedSuggestionsCallback;
using ntp_snippets::FakeCategoryRanker;
using ntp_snippets::FetchDoneCallback;
using ntp_snippets::ImageFetchedCallback;
using ntp_snippets::KnownCategories;
using ntp_snippets::RemoteSuggestion;
using ntp_snippets::UserClassifier;
using ntp_snippets::test::RemoteSuggestionBuilder;
using testing::_;
using testing::InSequence;
using testing::Return;

extern ApplicationState*
    g_content_suggestions_notification_application_state_for_testing;

namespace {

std::unique_ptr<sync_preferences::TestingPrefServiceSyncable>
RegisteredPrefs() {
  auto prefs = std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
  ContentSuggestionsService::RegisterProfilePrefs(prefs->registry());
  UserClassifier::RegisterProfilePrefs(prefs->registry());
  ContentSuggestionsNotifierService::RegisterProfilePrefs(prefs->registry());
  prefs->registry()->RegisterBooleanPref(
      ntp_snippets::prefs::kArticlesListVisible, true);
  return prefs;
}

class FakeContentSuggestionsService : public ContentSuggestionsService {
 public:
  FakeContentSuggestionsService(PrefService* prefs, base::Clock* clock)
      : ContentSuggestionsService(
            ContentSuggestionsService::State::ENABLED,
            /*identity_manager=*/nullptr,
            /*history_service=*/nullptr,
            /*large_icon_cache=*/nullptr,
            prefs,
            std::make_unique<FakeCategoryRanker>(),
            std::make_unique<UserClassifier>(nullptr, clock),
            /*remote_suggestions_scheduler=*/nullptr,
            std::make_unique<ntp_snippets::Logger>()) {}
};

class FakeArticleProvider : public ContentSuggestionsProvider {
 public:
  explicit FakeArticleProvider(ContentSuggestionsService* service)
      : ContentSuggestionsProvider(service) {}

  CategoryStatus GetCategoryStatus(Category category) override {
    if (category.IsKnownCategory(KnownCategories::ARTICLES)) {
      return CategoryStatus::AVAILABLE;
    }
    return CategoryStatus::NOT_PROVIDED;
  }

  CategoryInfo GetCategoryInfo(Category category) override {
    DCHECK(category.IsKnownCategory(KnownCategories::ARTICLES));
    CategoryInfo info(base::ASCIIToUTF16("title"),
                      ContentSuggestionsCardLayout::FULL_CARD,
                      ContentSuggestionsAdditionalAction::NONE, true,
                      base::ASCIIToUTF16("no suggestions"));
    return info;
  }

  void FetchSuggestionImage(const ContentSuggestion::ID& id,
                            ImageFetchedCallback callback) override {
    std::move(callback).Run(gfx::Image());
  }

  void FetchSuggestionImageData(
      const ContentSuggestion::ID& suggestion_id,
      ntp_snippets::ImageDataFetchedCallback callback) override {
    std::move(callback).Run(std::string());
  }

  void DismissSuggestion(const ContentSuggestion::ID& id) override {
    NOTIMPLEMENTED();
  }

  void Fetch(const Category& category,
             const std::set<std::string>& known_suggestion_ids,
             FetchDoneCallback callback) override {
    NOTIMPLEMENTED();
  }

  void ClearHistory(
      base::Time begin,
      base::Time end,
      const base::Callback<bool(const GURL& url)>& filter) override {
    NOTIMPLEMENTED();
  }

  void ClearCachedSuggestions() override { NOTIMPLEMENTED(); }

  void GetDismissedSuggestionsForDebugging(
      Category category,
      DismissedSuggestionsCallback callback) override {
    NOTIMPLEMENTED();
  }

  void ClearDismissedSuggestionsForDebugging(Category category) override {
    NOTIMPLEMENTED();
  }
};

class MockContentSuggestionsNotifier : public ContentSuggestionsNotifier {
 public:
  MOCK_METHOD0(CreateNotificationChannel, void());
  MOCK_METHOD7(SendNotification,
               bool(const ContentSuggestion::ID& id,
                    const GURL& url,
                    const base::string16& title,
                    const base::string16& text,
                    const gfx::Image& image,
                    base::Time timeout_at,
                    int priority));
  MOCK_METHOD2(HideNotification,
               void(const ContentSuggestion::ID& id,
                    ContentSuggestionsNotificationAction why));
  MOCK_METHOD1(HideAllNotifications,
               void(ContentSuggestionsNotificationAction why));
  MOCK_METHOD0(FlushCachedMetrics, void());
  MOCK_METHOD1(RegisterChannel, bool(bool enabled));
  MOCK_METHOD0(UnregisterChannel, void());
};

class ContentSuggestionsNotifierServiceTest : public ::testing::Test {
 protected:
  ContentSuggestionsNotifierServiceTest()
      : application_state_(APPLICATION_STATE_HAS_PAUSED_ACTIVITIES),
        prefs_(RegisteredPrefs()),
        suggestions_(prefs_.get(), &clock_),
        notifier_(new testing::StrictMock<MockContentSuggestionsNotifier>),
        notifier_ownership_(notifier_),
        provider_(&suggestions_) {
    g_content_suggestions_notification_application_state_for_testing =
        &application_state_;
  }

  ~ContentSuggestionsNotifierServiceTest() override {
    g_content_suggestions_notification_application_state_for_testing = nullptr;
  }

  void NewSuggestions(const std::vector<std::unique_ptr<RemoteSuggestion>>&
                          remote_suggestions) {
    Category articles = Category::FromKnownCategory(KnownCategories::ARTICLES);
    ContentSuggestionsProvider::Observer* observer = &suggestions_;
    std::vector<ContentSuggestion> content_suggestions;
    for (const auto& remote_suggestion : remote_suggestions) {
      content_suggestions.emplace_back(
          remote_suggestion->ToContentSuggestion(articles));
    }

    observer->OnNewSuggestions(&provider_, articles,
                               std::move(content_suggestions));
  }

  ApplicationState application_state_;
  std::unique_ptr<sync_preferences::TestingPrefServiceSyncable> prefs_;
  base::SimpleTestClock clock_;
  FakeContentSuggestionsService suggestions_;
  testing::StrictMock<MockContentSuggestionsNotifier>* notifier_;
  std::unique_ptr<ContentSuggestionsNotifier> notifier_ownership_;
  FakeArticleProvider provider_;
};

TEST_F(ContentSuggestionsNotifierServiceTest, ShouldInitializeAtStartup) {
  InSequence s;
  EXPECT_CALL(*notifier_, FlushCachedMetrics());
  EXPECT_CALL(*notifier_, RegisterChannel(true)).WillOnce(Return(false));
  ContentSuggestionsNotifierService service(prefs_.get(), &suggestions_,
                                            std::move(notifier_ownership_));
}

TEST_F(ContentSuggestionsNotifierServiceTest,
       ShouldNotInitializeAtStartupIfDisabled) {
  prefs_->SetBoolean(prefs::kContentSuggestionsNotificationsEnabled, false);

  InSequence s;
  EXPECT_CALL(*notifier_, FlushCachedMetrics());
  EXPECT_CALL(*notifier_, RegisterChannel(false)).WillOnce(Return(false));
  ContentSuggestionsNotifierService service(prefs_.get(), &suggestions_,
                                            std::move(notifier_ownership_));
}

TEST_F(ContentSuggestionsNotifierServiceTest,
       ShouldClearNotificationSettingAfterOUpgrade) {
  prefs_->SetBoolean(prefs::kContentSuggestionsNotificationsEnabled, false);

  InSequence s;
  EXPECT_CALL(*notifier_, FlushCachedMetrics());
  EXPECT_CALL(*notifier_, RegisterChannel(false))
      .WillOnce(Return(true));  // Channel actually registered
  ContentSuggestionsNotifierService service(prefs_.get(), &suggestions_,
                                            std::move(notifier_ownership_));

  // Now that notification channels are used, our setting should be true
  // unconditionally, as notification channels are used for settings instead.
  EXPECT_EQ(true,
            prefs_->GetBoolean(prefs::kContentSuggestionsNotificationsEnabled));
}

TEST_F(ContentSuggestionsNotifierServiceTest,
       ShouldNotSendNotificationIfNotShouldNotify) {
  InSequence s;
  EXPECT_CALL(*notifier_, FlushCachedMetrics());
  EXPECT_CALL(*notifier_, RegisterChannel(true)).WillOnce(Return(false));

  ContentSuggestionsNotifierService service(prefs_.get(), &suggestions_,
                                            std::move(notifier_ownership_));

  std::vector<std::unique_ptr<RemoteSuggestion>> suggestions;
  suggestions.emplace_back(
      RemoteSuggestionBuilder().SetShouldNotify(false).Build());

  // No notification.
  NewSuggestions(suggestions);
}

TEST_F(ContentSuggestionsNotifierServiceTest,
       ShouldSendNotificationIfShouldNotify) {
  InSequence s;
  EXPECT_CALL(*notifier_, FlushCachedMetrics());
  EXPECT_CALL(*notifier_, RegisterChannel(true)).WillOnce(Return(false));

  ContentSuggestionsNotifierService service(prefs_.get(), &suggestions_,
                                            std::move(notifier_ownership_));

  std::vector<std::unique_ptr<RemoteSuggestion>> suggestions;
  suggestions.emplace_back(
      RemoteSuggestionBuilder().SetShouldNotify(true).Build());

  EXPECT_CALL(*notifier_, SendNotification(_, _, _, _, _, _, _));
  NewSuggestions(suggestions);
}

TEST_F(ContentSuggestionsNotifierServiceTest,
       ShouldNotSendNotificationIfNotificationsDisabled) {
  prefs_->SetBoolean(prefs::kContentSuggestionsNotificationsEnabled, false);

  InSequence s;
  EXPECT_CALL(*notifier_, FlushCachedMetrics());
  EXPECT_CALL(*notifier_, RegisterChannel(false)).WillOnce(Return(false));

  ContentSuggestionsNotifierService service(prefs_.get(), &suggestions_,
                                            std::move(notifier_ownership_));

  std::vector<std::unique_ptr<RemoteSuggestion>> suggestions;
  suggestions.emplace_back(
      RemoteSuggestionBuilder().SetShouldNotify(true).Build());

  // No notification.
  NewSuggestions(suggestions);
}

}  // namespace
