// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/breaking_news/subscription_manager_impl.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/test/histogram_tester.h"
#include "build/build_config.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/ntp_snippets/remote/test_utils.h"
#include "components/prefs/testing_pref_service.h"
#include "net/base/net_errors.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "services/identity/public/cpp/identity_test_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

namespace ntp_snippets {

#if !defined(OS_CHROMEOS)
const char kTestEmail[] = "test@email.com";
#endif

const char kAPIKey[] = "fakeAPIkey";
const char kSubscriptionUrl[] = "http://valid-url.test/subscribe";
const char kSubscriptionUrlSignedIn[] = "http://valid-url.test/subscribe";
;
const char kSubscriptionUrlSignedOut[] =
    "http://valid-url.test/subscribe?key=fakeAPIkey";
const char kUnsubscriptionUrl[] = "http://valid-url.test/unsubscribe";
const char kUnsubscriptionUrlSignedIn[] = "http://valid-url.test/unsubscribe";
const char kUnsubscriptionUrlSignedOut[] =
    "http://valid-url.test/unsubscribe?key=fakeAPIkey";

class SubscriptionManagerImplTest : public testing::Test {
 public:
  SubscriptionManagerImplTest()
      : request_context_getter_(
            new net::TestURLRequestContextGetter(message_loop_.task_runner())) {
  }

  void SetUp() override {
    SubscriptionManagerImpl::RegisterProfilePrefs(
        utils_.pref_service()->registry());
  }

  scoped_refptr<net::URLRequestContextGetter> GetRequestContext() {
    return request_context_getter_.get();
  }

  PrefService* GetPrefService() { return utils_.pref_service(); }

  identity::IdentityTestEnvironment* GetIdentityTestEnv() {
    return &identity_test_env_;
  }

  std::unique_ptr<SubscriptionManagerImpl> BuildSubscriptionManager() {
    return std::make_unique<SubscriptionManagerImpl>(
        GetRequestContext(), GetPrefService(),
        /*variations_service=*/nullptr,
        GetIdentityTestEnv()->identity_manager(),
        /*locale=*/"", kAPIKey, GURL(kSubscriptionUrl),
        GURL(kUnsubscriptionUrl));
  }

  net::TestURLFetcher* GetRunningFetcher() {
    // All created TestURLFetchers have ID 0 by default.
    net::TestURLFetcher* url_fetcher = url_fetcher_factory_.GetFetcherByID(0);
    DCHECK(url_fetcher);
    return url_fetcher;
  }

  void RespondToSubscriptionRequestSuccessfully(bool is_signed_in) {
    net::TestURLFetcher* url_fetcher = GetRunningFetcher();
    if (is_signed_in) {
      ASSERT_EQ(GURL(kSubscriptionUrlSignedIn), url_fetcher->GetOriginalURL());
    } else {
      ASSERT_EQ(GURL(kSubscriptionUrlSignedOut), url_fetcher->GetOriginalURL());
    }
    RespondSuccessfully();
  }

  void RespondToUnsubscriptionRequestSuccessfully(bool is_signed_in) {
    net::TestURLFetcher* url_fetcher = GetRunningFetcher();
    if (is_signed_in) {
      ASSERT_EQ(GURL(kUnsubscriptionUrlSignedIn),
                url_fetcher->GetOriginalURL());
    } else {
      ASSERT_EQ(GURL(kUnsubscriptionUrlSignedOut),
                url_fetcher->GetOriginalURL());
    }
    RespondSuccessfully();
  }

  void RespondToSubscriptionWithError(bool is_signed_in, int error_code) {
    net::TestURLFetcher* url_fetcher = GetRunningFetcher();
    if (is_signed_in) {
      ASSERT_EQ(GURL(kSubscriptionUrlSignedIn), url_fetcher->GetOriginalURL());
    } else {
      ASSERT_EQ(GURL(kSubscriptionUrlSignedOut), url_fetcher->GetOriginalURL());
    }
    RespondWithError(error_code);
  }

  void RespondToUnsubscriptionWithError(bool is_signed_in, int error_code) {
    net::TestURLFetcher* url_fetcher = GetRunningFetcher();
    if (is_signed_in) {
      ASSERT_EQ(GURL(kUnsubscriptionUrlSignedIn),
                url_fetcher->GetOriginalURL());
    } else {
      ASSERT_EQ(GURL(kUnsubscriptionUrlSignedOut),
                url_fetcher->GetOriginalURL());
    }
    RespondWithError(error_code);
  }

#if !defined(OS_CHROMEOS)
  void SignIn() {
    GetIdentityTestEnv()->MakePrimaryAccountAvailable(kTestEmail);
  }

  void SignOut() { GetIdentityTestEnv()->ClearPrimaryAccount(); }
#endif  // !defined(OS_CHROMEOS)

 private:
  void RespondSuccessfully() {
    net::TestURLFetcher* url_fetcher = GetRunningFetcher();
    url_fetcher->set_status(net::URLRequestStatus());
    url_fetcher->set_response_code(net::HTTP_OK);
    url_fetcher->SetResponseString(std::string());
    // Call the URLFetcher delegate to continue the test.
    url_fetcher->delegate()->OnURLFetchComplete(url_fetcher);
  }

  void RespondWithError(int error_code) {
    net::TestURLFetcher* url_fetcher = GetRunningFetcher();
    url_fetcher->set_status(net::URLRequestStatus::FromError(error_code));
    url_fetcher->SetResponseString(std::string());
    // Call the URLFetcher delegate to continue the test.
    url_fetcher->delegate()->OnURLFetchComplete(url_fetcher);
  }

  base::MessageLoop message_loop_;
  test::RemoteSuggestionsTestUtils utils_;
  identity::IdentityTestEnvironment identity_test_env_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;
  net::TestURLFetcherFactory url_fetcher_factory_;
};

TEST_F(SubscriptionManagerImplTest, SubscribeSuccessfully) {
  std::string subscription_token = "1234567890";
  std::unique_ptr<SubscriptionManagerImpl> manager = BuildSubscriptionManager();
  manager->Subscribe(subscription_token);
  RespondToSubscriptionRequestSuccessfully(/*is_signed_in=*/false);
  ASSERT_TRUE(manager->IsSubscribed());
  EXPECT_EQ(subscription_token, GetPrefService()->GetString(
                                    prefs::kBreakingNewsSubscriptionDataToken));
  EXPECT_FALSE(GetPrefService()->GetBoolean(
      prefs::kBreakingNewsSubscriptionDataIsAuthenticated));
}

// This test is relevant only on non-ChromeOS platforms, as the flow being
// tested here is not possible on ChromeOS.
#if !defined(OS_CHROMEOS)
TEST_F(SubscriptionManagerImplTest,
       ShouldSubscribeWithAuthenticationWhenAuthenticated) {
  // Sign in.
  SignIn();

  // Create manager and subscribe.
  std::string subscription_token = "1234567890";
  std::unique_ptr<SubscriptionManagerImpl> manager = BuildSubscriptionManager();
  manager->Subscribe(subscription_token);

  // Wait for the access token request and issue the access token.
  GetIdentityTestEnv()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
      "access_token", base::Time::Max());

  ASSERT_FALSE(manager->IsSubscribed());
  RespondToSubscriptionRequestSuccessfully(/*is_signed_in=*/true);
  ASSERT_TRUE(manager->IsSubscribed());

  // Check that we are now subscribed correctly with authentication.
  EXPECT_EQ(subscription_token, GetPrefService()->GetString(
                                    prefs::kBreakingNewsSubscriptionDataToken));
  EXPECT_TRUE(GetPrefService()->GetBoolean(
      prefs::kBreakingNewsSubscriptionDataIsAuthenticated));
}
#endif

TEST_F(SubscriptionManagerImplTest, ShouldNotSubscribeIfError) {
  std::string subscription_token = "1234567890";
  std::unique_ptr<SubscriptionManagerImpl> manager = BuildSubscriptionManager();

  manager->Subscribe(subscription_token);
  RespondToSubscriptionWithError(/*is_signed_in=*/false, net::ERR_TIMED_OUT);
  EXPECT_FALSE(manager->IsSubscribed());
}

TEST_F(SubscriptionManagerImplTest, UnsubscribeSuccessfully) {
  std::string subscription_token = "1234567890";
  std::unique_ptr<SubscriptionManagerImpl> manager = BuildSubscriptionManager();
  manager->Subscribe(subscription_token);
  RespondToSubscriptionRequestSuccessfully(/*is_signed_in=*/false);
  ASSERT_TRUE(manager->IsSubscribed());
  manager->Unsubscribe();
  RespondToUnsubscriptionRequestSuccessfully(/*is_signed_in=*/false);
  EXPECT_FALSE(manager->IsSubscribed());
  EXPECT_FALSE(
      GetPrefService()->HasPrefPath(prefs::kBreakingNewsSubscriptionDataToken));
}

TEST_F(SubscriptionManagerImplTest,
       ShouldRemainSubscribedIfErrorDuringUnsubscribe) {
  std::string subscription_token = "1234567890";
  std::unique_ptr<SubscriptionManagerImpl> manager = BuildSubscriptionManager();
  manager->Subscribe(subscription_token);
  RespondToSubscriptionRequestSuccessfully(/*is_signed_in=*/false);
  ASSERT_TRUE(manager->IsSubscribed());
  manager->Unsubscribe();
  RespondToUnsubscriptionWithError(/*is_signed_in=*/false, net::ERR_TIMED_OUT);
  ASSERT_TRUE(manager->IsSubscribed());
  EXPECT_EQ(subscription_token, GetPrefService()->GetString(
                                    prefs::kBreakingNewsSubscriptionDataToken));
}

// This test is relevant only on non-ChromeOS platforms, as the flow being
// tested here is not possible on ChromeOS.
#if !defined(OS_CHROMEOS)
TEST_F(SubscriptionManagerImplTest,
       ShouldResubscribeIfSignInAfterSubscription) {
  // Create manager and subscribe.
  std::string subscription_token = "1234567890";
  std::unique_ptr<SubscriptionManagerImpl> manager = BuildSubscriptionManager();
  manager->Subscribe(subscription_token);
  RespondToSubscriptionRequestSuccessfully(/*is_signed_in=*/false);
  ASSERT_FALSE(manager->NeedsToResubscribe());

  // Sign in. This should trigger a resubscribe.
  SignIn();
  ASSERT_TRUE(manager->NeedsToResubscribe());

  // Wait for the access token request that should occur and grant the access
  // token.
  GetIdentityTestEnv()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
      "access_token", base::Time::Max());

  RespondToSubscriptionRequestSuccessfully(/*is_signed_in=*/true);

  // Check that we are now subscribed with authentication.
  EXPECT_TRUE(GetPrefService()->GetBoolean(
      prefs::kBreakingNewsSubscriptionDataIsAuthenticated));
}
#endif

// This test is relevant only on non-ChromeOS platforms, as the flow being
// tested here is not possible on ChromeOS.
#if !defined(OS_CHROMEOS)
TEST_F(SubscriptionManagerImplTest,
       ShouldResubscribeIfSignOutAfterSubscription) {
  // Signin and subscribe.
  SignIn();
  std::string subscription_token = "1234567890";
  std::unique_ptr<SubscriptionManagerImpl> manager = BuildSubscriptionManager();
  manager->Subscribe(subscription_token);

  GetIdentityTestEnv()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
      "access_token", base::Time::Max());
  RespondToSubscriptionRequestSuccessfully(/*is_signed_in=*/true);

  // Sign out; this should trigger a resubscribe.
  SignOut();
  EXPECT_TRUE(manager->NeedsToResubscribe());
  RespondToSubscriptionRequestSuccessfully(/*is_signed_in=*/false);

  // Check that we are now subscribed without authentication.
  EXPECT_FALSE(GetPrefService()->GetBoolean(
      prefs::kBreakingNewsSubscriptionDataIsAuthenticated));
}
#endif

TEST_F(SubscriptionManagerImplTest,
       ShouldUpdateTokenInPrefWhenResubscribeWithChangeInToken) {
  // Create manager and subscribe.
  std::string old_subscription_token = "1234567890";
  std::unique_ptr<SubscriptionManagerImpl> manager = BuildSubscriptionManager();
  manager->Subscribe(old_subscription_token);
  RespondToSubscriptionRequestSuccessfully(/*is_signed_in=*/false);
  EXPECT_EQ(
      old_subscription_token,
      GetPrefService()->GetString(prefs::kBreakingNewsSubscriptionDataToken));

  // Resubscribe with a new token.
  std::string new_subscription_token = "0987654321";
  manager->Resubscribe(new_subscription_token);
  // Resubscribe with a new token should issue an unsubscribe request before
  // subscribing.
  RespondToUnsubscriptionRequestSuccessfully(/*is_signed_in=*/false);
  RespondToSubscriptionRequestSuccessfully(/*is_signed_in=*/false);

  // Check we are now subscribed with the new token.
  EXPECT_EQ(
      new_subscription_token,
      GetPrefService()->GetString(prefs::kBreakingNewsSubscriptionDataToken));
}

TEST_F(SubscriptionManagerImplTest, ShouldReportSubscriptionResult) {
  base::HistogramTester histogram_tester;
  // Create manager and subscribe.
  const std::string subscription_token = "token";
  std::unique_ptr<SubscriptionManagerImpl> manager = BuildSubscriptionManager();
  manager->Subscribe(subscription_token);
  // TODO(vitaliii): Mock subscription request to avoid this low level errors.
  RespondToSubscriptionWithError(/*is_signed_in=*/false,
                                 /*error_code=*/net::ERR_INVALID_RESPONSE);

  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.ContentSuggestions."
                                     "BreakingNews.SubscriptionRequestStatus"),
      ElementsAre(base::Bucket(
          /*min=*/static_cast<int>(StatusCode::TEMPORARY_ERROR),
          /*count=*/1)));
}

TEST_F(SubscriptionManagerImplTest, ShouldReportUnsubscriptionResult) {
  base::HistogramTester histogram_tester;
  // Create manager and subscribe.
  const std::string subscription_token = "token";
  std::unique_ptr<SubscriptionManagerImpl> manager = BuildSubscriptionManager();
  manager->Subscribe(subscription_token);
  RespondToSubscriptionRequestSuccessfully(/*is_signed_in=*/false);
  manager->Unsubscribe();

  RespondToUnsubscriptionWithError(/*is_signed_in=*/false,
                                   /*error_code=*/net::ERR_INVALID_RESPONSE);
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.ContentSuggestions."
                                             "BreakingNews."
                                             "UnsubscriptionRequestStatus"),
              ElementsAre(base::Bucket(
                  /*min=*/static_cast<int>(StatusCode::TEMPORARY_ERROR),
                  /*count=*/1)));
}

}  // namespace ntp_snippets
