// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/one_google_bar/one_google_bar_service.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/optional.h"
#include "base/test/scoped_task_environment.h"
#include "chrome/browser/search/one_google_bar/one_google_bar_data.h"
#include "chrome/browser/search/one_google_bar/one_google_bar_loader.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_gaia_cookie_manager_service.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "google_apis/gaia/fake_oauth2_token_service.h"
#include "google_apis/gaia/gaia_constants.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Eq;
using testing::InSequence;
using testing::StrictMock;

class FakeOneGoogleBarLoader : public OneGoogleBarLoader {
 public:
  void Load(OneGoogleCallback callback) override {
    callbacks_.push_back(std::move(callback));
  }

  GURL GetLoadURLForTesting() const override { return GURL(); }

  size_t GetCallbackCount() const { return callbacks_.size(); }

  void RespondToAllCallbacks(Status status,
                             const base::Optional<OneGoogleBarData>& data) {
    for (OneGoogleCallback& callback : callbacks_) {
      std::move(callback).Run(status, data);
    }
    callbacks_.clear();
  }

 private:
  std::vector<OneGoogleCallback> callbacks_;
};

class MockOneGoogleBarServiceObserver : public OneGoogleBarServiceObserver {
 public:
  MOCK_METHOD0(OnOneGoogleBarDataUpdated, void());
};

class OneGoogleBarServiceTest : public testing::Test {
 public:
  OneGoogleBarServiceTest()
      : signin_client_(&pref_service_),
        fetcher_factory_(/*default_factory=*/nullptr),
        cookie_service_(&token_service_,
                        GaiaConstants::kChromeSource,
                        &signin_client_) {
    // GaiaCookieManagerService calls static methods of AccountTrackerService
    // which access prefs.
    AccountTrackerService::RegisterPrefs(pref_service_.registry());

    cookie_service_.Init(&fetcher_factory_);

    auto loader = std::make_unique<FakeOneGoogleBarLoader>();
    loader_ = loader.get();
    service_ = std::make_unique<OneGoogleBarService>(&cookie_service_,
                                                     std::move(loader));
  }

  FakeOneGoogleBarLoader* loader() { return loader_; }
  OneGoogleBarService* service() { return service_.get(); }

  void SignIn() {
    cookie_service_.SetListAccountsResponseOneAccount("user@gmail.com",
                                                      "gaia_id");
    cookie_service_.TriggerListAccounts(GaiaConstants::kChromeSource);
    task_environment_.RunUntilIdle();
  }

  void SignOut() {
    cookie_service_.SetListAccountsResponseNoAccounts();
    cookie_service_.TriggerListAccounts(GaiaConstants::kChromeSource);
    task_environment_.RunUntilIdle();
  }

 private:
  base::test::ScopedTaskEnvironment task_environment_;

  sync_preferences::TestingPrefServiceSyncable pref_service_;
  TestSigninClient signin_client_;
  FakeOAuth2TokenService token_service_;
  net::FakeURLFetcherFactory fetcher_factory_;
  FakeGaiaCookieManagerService cookie_service_;

  // Owned by the service.
  FakeOneGoogleBarLoader* loader_;

  std::unique_ptr<OneGoogleBarService> service_;
};

TEST_F(OneGoogleBarServiceTest, RefreshesOnRequest) {
  ASSERT_THAT(service()->one_google_bar_data(), Eq(base::nullopt));

  // Request a refresh. That should arrive at the loader.
  service()->Refresh();
  EXPECT_THAT(loader()->GetCallbackCount(), Eq(1u));

  // Fulfill it.
  OneGoogleBarData data;
  data.bar_html = "<div></div>";
  loader()->RespondToAllCallbacks(OneGoogleBarLoader::Status::OK, data);
  EXPECT_THAT(service()->one_google_bar_data(), Eq(data));

  // Request another refresh.
  service()->Refresh();
  EXPECT_THAT(loader()->GetCallbackCount(), Eq(1u));

  // For now, the old data should still be there.
  EXPECT_THAT(service()->one_google_bar_data(), Eq(data));

  // Fulfill the second request.
  OneGoogleBarData other_data;
  other_data.bar_html = "<div>Different!</div>";
  loader()->RespondToAllCallbacks(OneGoogleBarLoader::Status::OK, other_data);
  EXPECT_THAT(service()->one_google_bar_data(), Eq(other_data));
}

TEST_F(OneGoogleBarServiceTest, NotifiesObserverOnChanges) {
  InSequence s;

  ASSERT_THAT(service()->one_google_bar_data(), Eq(base::nullopt));

  StrictMock<MockOneGoogleBarServiceObserver> observer;
  service()->AddObserver(&observer);

  // Empty result from a fetch should result in a notification.
  service()->Refresh();
  EXPECT_CALL(observer, OnOneGoogleBarDataUpdated());
  loader()->RespondToAllCallbacks(OneGoogleBarLoader::Status::OK,
                                  base::nullopt);
  EXPECT_THAT(service()->one_google_bar_data(), Eq(base::nullopt));

  // Non-empty response should result in a notification.
  service()->Refresh();
  OneGoogleBarData data;
  data.bar_html = "<div></div>";
  EXPECT_CALL(observer, OnOneGoogleBarDataUpdated());
  loader()->RespondToAllCallbacks(OneGoogleBarLoader::Status::OK, data);
  EXPECT_THAT(service()->one_google_bar_data(), Eq(data));

  // Identical response should still result in a notification.
  service()->Refresh();
  OneGoogleBarData identical_data = data;
  EXPECT_CALL(observer, OnOneGoogleBarDataUpdated());
  loader()->RespondToAllCallbacks(OneGoogleBarLoader::Status::OK,
                                  identical_data);
  EXPECT_THAT(service()->one_google_bar_data(), Eq(data));

  // Different response should result in a notification.
  service()->Refresh();
  OneGoogleBarData other_data;
  data.bar_html = "<div>Different</div>";
  EXPECT_CALL(observer, OnOneGoogleBarDataUpdated());
  loader()->RespondToAllCallbacks(OneGoogleBarLoader::Status::OK, other_data);
  EXPECT_THAT(service()->one_google_bar_data(), Eq(other_data));

  service()->RemoveObserver(&observer);
}

TEST_F(OneGoogleBarServiceTest, KeepsCacheOnTransientError) {
  // Load some data.
  service()->Refresh();
  OneGoogleBarData data;
  data.bar_html = "<div></div>";
  loader()->RespondToAllCallbacks(OneGoogleBarLoader::Status::OK, data);
  ASSERT_THAT(service()->one_google_bar_data(), Eq(data));

  StrictMock<MockOneGoogleBarServiceObserver> observer;
  service()->AddObserver(&observer);

  // Request a refresh and respond with a transient error.
  service()->Refresh();
  EXPECT_CALL(observer, OnOneGoogleBarDataUpdated());
  loader()->RespondToAllCallbacks(OneGoogleBarLoader::Status::TRANSIENT_ERROR,
                                  base::nullopt);
  // Cached data should still be there.
  EXPECT_THAT(service()->one_google_bar_data(), Eq(data));

  service()->RemoveObserver(&observer);
}

TEST_F(OneGoogleBarServiceTest, ClearsCacheOnFatalError) {
  // Load some data.
  service()->Refresh();
  OneGoogleBarData data;
  data.bar_html = "<div></div>";
  loader()->RespondToAllCallbacks(OneGoogleBarLoader::Status::OK, data);
  ASSERT_THAT(service()->one_google_bar_data(), Eq(data));

  StrictMock<MockOneGoogleBarServiceObserver> observer;
  service()->AddObserver(&observer);

  // Request a refresh and respond with a fatal error.
  service()->Refresh();
  EXPECT_CALL(observer, OnOneGoogleBarDataUpdated());
  loader()->RespondToAllCallbacks(OneGoogleBarLoader::Status::FATAL_ERROR,
                                  base::nullopt);
  // Cached data should be gone now.
  EXPECT_THAT(service()->one_google_bar_data(), Eq(base::nullopt));

  service()->RemoveObserver(&observer);
}

TEST_F(OneGoogleBarServiceTest, ResetsOnSignIn) {
  // Load some data.
  service()->Refresh();
  OneGoogleBarData data;
  data.bar_html = "<div></div>";
  loader()->RespondToAllCallbacks(OneGoogleBarLoader::Status::OK, data);
  ASSERT_THAT(service()->one_google_bar_data(), Eq(data));

  StrictMock<MockOneGoogleBarServiceObserver> observer;
  service()->AddObserver(&observer);

  // Sign in. This should clear the cached data and notify the observer.
  EXPECT_CALL(observer, OnOneGoogleBarDataUpdated());
  SignIn();
  EXPECT_THAT(service()->one_google_bar_data(), Eq(base::nullopt));

  service()->RemoveObserver(&observer);
}

TEST_F(OneGoogleBarServiceTest, ResetsOnSignOut) {
  SignIn();

  // Load some data.
  service()->Refresh();
  OneGoogleBarData data;
  data.bar_html = "<div></div>";
  loader()->RespondToAllCallbacks(OneGoogleBarLoader::Status::OK, data);
  ASSERT_THAT(service()->one_google_bar_data(), Eq(data));

  StrictMock<MockOneGoogleBarServiceObserver> observer;
  service()->AddObserver(&observer);

  // Sign in. This should clear the cached data and notify the observer.
  EXPECT_CALL(observer, OnOneGoogleBarDataUpdated());
  SignOut();
  EXPECT_THAT(service()->one_google_bar_data(), Eq(base::nullopt));

  service()->RemoveObserver(&observer);
}

TEST_F(OneGoogleBarServiceTest, DoesNotNotifyObserverOnSignInIfNoCachedData) {
  ASSERT_THAT(service()->one_google_bar_data(), Eq(base::nullopt));

  StrictMock<MockOneGoogleBarServiceObserver> observer;
  service()->AddObserver(&observer);

  // Sign in. This should *not* notify the observer, since there was no cached
  // data before.
  SignIn();
  EXPECT_THAT(service()->one_google_bar_data(), Eq(base::nullopt));

  service()->RemoveObserver(&observer);
}
