// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/sessions/foreign_sessions_suggestions_provider.h"

#include <map>
#include <utility>

#include "base/callback_forward.h"
#include "base/strings/string_number_conversions.h"
#include "components/ntp_snippets/category.h"
#include "components/ntp_snippets/content_suggestions_provider.h"
#include "components/ntp_snippets/mock_content_suggestions_provider_observer.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "components/sessions/core/serialized_navigation_entry_test_helper.h"
#include "components/sessions/core/session_types.h"
#include "components/sync_sessions/synced_session.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::Time;
using base::TimeDelta;
using sessions::SerializedNavigationEntry;
using sessions::SessionTab;
using sessions::SessionWindow;
using sync_sessions::SyncedSession;
using sync_sessions::SyncedSessionWindow;
using testing::ElementsAre;
using testing::IsEmpty;
using testing::Property;
using testing::Test;
using testing::_;

namespace ntp_snippets {
namespace {

const char kUrl1[] = "http://www.fake1.com/";
const char kUrl2[] = "http://www.fake2.com/";
const char kUrl3[] = "http://www.fake3.com/";
const char kUrl4[] = "http://www.fake4.com/";
const char kUrl5[] = "http://www.fake5.com/";
const char kUrl6[] = "http://www.fake6.com/";
const char kUrl7[] = "http://www.fake7.com/";
const char kUrl8[] = "http://www.fake8.com/";
const char kUrl9[] = "http://www.fake9.com/";
const char kUrl10[] = "http://www.fake10.com/";
const char kUrl11[] = "http://www.fake11.com/";
const char kTitle[] = "title is ignored";

SessionWindow* GetOrCreateWindow(SyncedSession* session, int window_id) {
  SessionID id = SessionID::FromSerializedValue(window_id);
  if (session->windows.find(id) == session->windows.end()) {
    session->windows[id] = std::make_unique<SyncedSessionWindow>();
  }

  return &session->windows[id]->wrapped_window;
}

void AddTabToSession(SyncedSession* session,
                     int window_id,
                     const std::string& url,
                     TimeDelta age) {
  SerializedNavigationEntry navigation =
      sessions::SerializedNavigationEntryTestHelper::CreateNavigation(url,
                                                                      kTitle);

  std::unique_ptr<SessionTab> tab = std::make_unique<SessionTab>();
  tab->timestamp = Time::Now() - age;
  tab->navigations.push_back(navigation);

  SessionWindow* window = GetOrCreateWindow(session, window_id);
  // The window deletes the tabs it points at upon destruction.
  window->tabs.push_back(std::move(tab));
}

class FakeForeignSessionsProvider : public ForeignSessionsProvider {
 public:
  ~FakeForeignSessionsProvider() override = default;
  void SetAllForeignSessions(std::vector<const SyncedSession*> sessions) {
    sessions_ = std::move(sessions);
    change_callback_.Run();
  }

  // ForeignSessionsProvider implementation.
  void SubscribeForForeignTabChange(
      const base::Closure& change_callback) override {
    change_callback_ = change_callback;
  }
  bool HasSessionsData() override { return true; }
  std::vector<const sync_sessions::SyncedSession*> GetAllForeignSessions()
      override {
    return sessions_;
  }

 private:
  std::vector<const SyncedSession*> sessions_;
  base::Closure change_callback_;
};
}  // namespace

class ForeignSessionsSuggestionsProviderTest : public Test {
 public:
  ForeignSessionsSuggestionsProviderTest() {
    ForeignSessionsSuggestionsProvider::RegisterProfilePrefs(
        pref_service_.registry());

    std::unique_ptr<FakeForeignSessionsProvider>
        fake_foreign_sessions_provider =
            std::make_unique<FakeForeignSessionsProvider>();
    fake_foreign_sessions_provider_ = fake_foreign_sessions_provider.get();

    // During the provider's construction the following mock calls occur.
    EXPECT_CALL(*observer(), OnNewSuggestions(_, category(), IsEmpty()));
    EXPECT_CALL(*observer(), OnCategoryStatusChanged(
                                 _, category(), CategoryStatus::AVAILABLE));

    provider_ = std::make_unique<ForeignSessionsSuggestionsProvider>(
        &observer_, std::move(fake_foreign_sessions_provider), &pref_service_);
  }

 protected:
  SyncedSession* GetOrCreateSession(int session_id) {
    if (sessions_map_.find(session_id) == sessions_map_.end()) {
      std::string id_as_string = base::IntToString(session_id);
      std::unique_ptr<SyncedSession> owned_session =
          std::make_unique<SyncedSession>();
      owned_session->session_tag = id_as_string;
      owned_session->session_name = id_as_string;
      sessions_map_[session_id] = std::move(owned_session);
    }
    return sessions_map_[session_id].get();
  }

  void AddTab(int session_id,
              int window_id,
              const std::string& url,
              TimeDelta age) {
    AddTabToSession(GetOrCreateSession(session_id), window_id, url, age);
  }

  void ClearSessionData() { sessions_map_.clear(); }

  void TriggerOnChange() {
    std::vector<const SyncedSession*> sessions;
    for (const auto& kv : sessions_map_) {
      sessions.push_back(kv.second.get());
    }
    fake_foreign_sessions_provider_->SetAllForeignSessions(std::move(sessions));
  }

  void Dismiss(const std::string& url) {
    // The url of a given suggestion is used as the |id_within_category|.
    provider_->DismissSuggestion(ContentSuggestion::ID(category(), url));
  }

  Category category() {
    return Category::FromKnownCategory(KnownCategories::FOREIGN_TABS);
  }

  MockContentSuggestionsProviderObserver* observer() { return &observer_; }

 private:
  FakeForeignSessionsProvider* fake_foreign_sessions_provider_;
  MockContentSuggestionsProviderObserver observer_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<ForeignSessionsSuggestionsProvider> provider_;
  std::map<int, std::unique_ptr<SyncedSession>> sessions_map_;

  DISALLOW_COPY_AND_ASSIGN(ForeignSessionsSuggestionsProviderTest);
};

TEST_F(ForeignSessionsSuggestionsProviderTest, Empty) {
  // When no sessions data is added, expect no suggestions.
  EXPECT_CALL(*observer(), OnNewSuggestions(_, category(), IsEmpty()));
  TriggerOnChange();
}

TEST_F(ForeignSessionsSuggestionsProviderTest, Single) {
  // Expect a single valid tab because that is what has been added.
  EXPECT_CALL(*observer(),
              OnNewSuggestions(
                  _, category(),
                  ElementsAre(Property(&ContentSuggestion::url, GURL(kUrl1)))));
  AddTab(0, 0, kUrl1, TimeDelta::FromMinutes(1));
  TriggerOnChange();
}

TEST_F(ForeignSessionsSuggestionsProviderTest, Old) {
  // The only sessions data is too old to be suggested, so expect empty.
  EXPECT_CALL(*observer(), OnNewSuggestions(_, category(), IsEmpty()));
  AddTab(0, 0, kUrl1, TimeDelta::FromHours(4));
  TriggerOnChange();
}

TEST_F(ForeignSessionsSuggestionsProviderTest, Ordered) {
  // Suggestions ordering should be in reverse chronological order, or youngest
  // first.
  EXPECT_CALL(*observer(),
              OnNewSuggestions(
                  _, category(),
                  ElementsAre(Property(&ContentSuggestion::url, GURL(kUrl1)),
                              Property(&ContentSuggestion::url, GURL(kUrl2)),
                              Property(&ContentSuggestion::url, GURL(kUrl3)),
                              Property(&ContentSuggestion::url, GURL(kUrl4)))));
  AddTab(0, 0, kUrl2, TimeDelta::FromMinutes(2));
  AddTab(0, 0, kUrl4, TimeDelta::FromMinutes(4));
  AddTab(0, 1, kUrl3, TimeDelta::FromMinutes(3));
  AddTab(1, 0, kUrl1, TimeDelta::FromMinutes(1));
  TriggerOnChange();
}

TEST_F(ForeignSessionsSuggestionsProviderTest, MaxPerDevice) {
  // Each device, which is to equivalent a unique |session_tag|, has a limit to
  // the number of suggestions it is allowed to contribute. Here all four
  // suggestions are within the recency threshold, but only three are allowed
  // per device. As such, expect that the oldest of the four will not be
  // suggested.
  EXPECT_CALL(*observer(),
              OnNewSuggestions(
                  _, category(),
                  ElementsAre(Property(&ContentSuggestion::url, GURL(kUrl1)),
                              Property(&ContentSuggestion::url, GURL(kUrl2)),
                              Property(&ContentSuggestion::url, GURL(kUrl3)))));
  AddTab(0, 0, kUrl1, TimeDelta::FromMinutes(1));
  AddTab(0, 0, kUrl2, TimeDelta::FromMinutes(2));
  AddTab(0, 0, kUrl3, TimeDelta::FromMinutes(3));
  AddTab(0, 0, kUrl4, TimeDelta::FromMinutes(4));
  TriggerOnChange();
}

TEST_F(ForeignSessionsSuggestionsProviderTest, MaxTotal) {
  // There's a limit to the total nubmer of suggestions that the provider will
  // ever return, which should be ten. Here there are eleven valid suggestion
  // entries, spread out over multiple devices/sessions to avoid the per device
  // cutoff. Expect that the least recent of the eleven to be dropped.
  EXPECT_CALL(
      *observer(),
      OnNewSuggestions(
          _, category(),
          ElementsAre(Property(&ContentSuggestion::url, GURL(kUrl1)),
                      Property(&ContentSuggestion::url, GURL(kUrl2)),
                      Property(&ContentSuggestion::url, GURL(kUrl3)),
                      Property(&ContentSuggestion::url, GURL(kUrl4)),
                      Property(&ContentSuggestion::url, GURL(kUrl5)),
                      Property(&ContentSuggestion::url, GURL(kUrl6)),
                      Property(&ContentSuggestion::url, GURL(kUrl7)),
                      Property(&ContentSuggestion::url, GURL(kUrl8)),
                      Property(&ContentSuggestion::url, GURL(kUrl9)),
                      Property(&ContentSuggestion::url, GURL(kUrl10)))));
  AddTab(0, 0, kUrl1, TimeDelta::FromMinutes(1));
  AddTab(0, 0, kUrl2, TimeDelta::FromMinutes(2));
  AddTab(0, 0, kUrl3, TimeDelta::FromMinutes(3));
  AddTab(1, 0, kUrl4, TimeDelta::FromMinutes(4));
  AddTab(1, 0, kUrl5, TimeDelta::FromMinutes(5));
  AddTab(1, 0, kUrl6, TimeDelta::FromMinutes(6));
  AddTab(2, 0, kUrl7, TimeDelta::FromMinutes(7));
  AddTab(2, 0, kUrl8, TimeDelta::FromMinutes(8));
  AddTab(2, 0, kUrl9, TimeDelta::FromMinutes(9));
  AddTab(3, 0, kUrl10, TimeDelta::FromMinutes(10));
  AddTab(3, 0, kUrl11, TimeDelta::FromMinutes(11));
  TriggerOnChange();
}

TEST_F(ForeignSessionsSuggestionsProviderTest, Duplicates) {
  // The same url is never suggested more than once at a time. All the session
  // data has the same url so only expect a single suggestion.
  EXPECT_CALL(*observer(),
              OnNewSuggestions(
                  _, category(),
                  ElementsAre(Property(&ContentSuggestion::url, GURL(kUrl1)))));
  AddTab(0, 0, kUrl1, TimeDelta::FromMinutes(1));
  AddTab(0, 1, kUrl1, TimeDelta::FromMinutes(2));
  AddTab(1, 1, kUrl1, TimeDelta::FromMinutes(3));
  TriggerOnChange();
}

TEST_F(ForeignSessionsSuggestionsProviderTest, DuplicatesChangingOtherSession) {
  // Normally |kUrl4| wouldn't show up, because session_id=0 already has 3
  // younger tabs, but session_id=1 has a younger |kUrl3| which gives |kUrl4| a
  // spot.
  EXPECT_CALL(*observer(),
              OnNewSuggestions(
                  _, category(),
                  ElementsAre(Property(&ContentSuggestion::url, GURL(kUrl3)),
                              Property(&ContentSuggestion::url, GURL(kUrl1)),
                              Property(&ContentSuggestion::url, GURL(kUrl2)),
                              Property(&ContentSuggestion::url, GURL(kUrl4)))));
  AddTab(0, 0, kUrl1, TimeDelta::FromMinutes(1));
  AddTab(0, 0, kUrl2, TimeDelta::FromMinutes(2));
  AddTab(0, 0, kUrl3, TimeDelta::FromMinutes(3));
  AddTab(0, 0, kUrl4, TimeDelta::FromMinutes(4));
  AddTab(1, 0, kUrl3, TimeDelta::FromMinutes(0));
  TriggerOnChange();
}

TEST_F(ForeignSessionsSuggestionsProviderTest, Dismissed) {
  // Dimissed urls should not be suggested.
  EXPECT_CALL(*observer(), OnNewSuggestions(_, category(), IsEmpty()));
  Dismiss(kUrl1);
  AddTab(0, 0, kUrl1, TimeDelta::FromMinutes(1));
  TriggerOnChange();
}

TEST_F(ForeignSessionsSuggestionsProviderTest, DismissedChangingOwnSession) {
  // Similar to DuplicatesChangingOtherSession, without dismissals we would
  // expect urls 1-3. However, because of dismissals we reach all the down to
  // |kUrl5| before the per device cutoff is hit.
  EXPECT_CALL(*observer(),
              OnNewSuggestions(
                  _, category(),
                  ElementsAre(Property(&ContentSuggestion::url, GURL(kUrl2)),
                              Property(&ContentSuggestion::url, GURL(kUrl3)),
                              Property(&ContentSuggestion::url, GURL(kUrl5)))));
  Dismiss(kUrl1);
  Dismiss(kUrl4);
  AddTab(0, 0, kUrl1, TimeDelta::FromMinutes(1));
  AddTab(0, 0, kUrl2, TimeDelta::FromMinutes(2));
  AddTab(0, 0, kUrl3, TimeDelta::FromMinutes(3));
  AddTab(0, 0, kUrl4, TimeDelta::FromMinutes(4));
  AddTab(0, 0, kUrl5, TimeDelta::FromMinutes(5));
  AddTab(0, 0, kUrl6, TimeDelta::FromMinutes(6));
  TriggerOnChange();
}

TEST_F(ForeignSessionsSuggestionsProviderTest, DismissedPruning) {
  EXPECT_CALL(*observer(),
              OnNewSuggestions(
                  _, category(),
                  ElementsAre(Property(&ContentSuggestion::url, GURL(kUrl1)),
                              Property(&ContentSuggestion::url, GURL(kUrl2)),
                              Property(&ContentSuggestion::url, GURL(kUrl3)))));
  AddTab(0, 0, kUrl1, TimeDelta::FromMinutes(1));
  AddTab(0, 0, kUrl2, TimeDelta::FromMinutes(2));
  AddTab(0, 0, kUrl3, TimeDelta::FromMinutes(3));
  TriggerOnChange();

  EXPECT_CALL(*observer(),
              OnNewSuggestions(
                  _, category(),
                  ElementsAre(Property(&ContentSuggestion::url, GURL(kUrl1)),
                              Property(&ContentSuggestion::url, GURL(kUrl3)))));
  Dismiss(kUrl2);
  TriggerOnChange();

  // This case is important because it verifies the dismissal of |kUrl2| from
  // above is still around.
  EXPECT_CALL(*observer(),
              OnNewSuggestions(
                  _, category(),
                  ElementsAre(Property(&ContentSuggestion::url, GURL(kUrl1)))));
  Dismiss(kUrl3);
  TriggerOnChange();

  EXPECT_CALL(*observer(),
              OnNewSuggestions(
                  _, category(),
                  ElementsAre(Property(&ContentSuggestion::url, GURL(kUrl1)))));
  // kUrl2 is no longer present, which should result in the dismissal being
  // pruned during the next round of suggestions.
  ClearSessionData();
  AddTab(0, 0, kUrl1, TimeDelta::FromMinutes(1));
  TriggerOnChange();

  // Verify that kUrl2 is now allowed, and the previous dismissal was pruned.
  EXPECT_CALL(*observer(),
              OnNewSuggestions(
                  _, category(),
                  ElementsAre(Property(&ContentSuggestion::url, GURL(kUrl1)),
                              Property(&ContentSuggestion::url, GURL(kUrl2)))));
  AddTab(0, 0, kUrl2, TimeDelta::FromMinutes(2));
  TriggerOnChange();
}

}  // namespace ntp_snippets
