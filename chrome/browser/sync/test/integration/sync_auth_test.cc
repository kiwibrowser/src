// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/sync/test/integration/bookmarks_helper.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/single_client_status_change_checker.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "chrome/browser/sync/test/integration/updated_progress_marker_checker.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/sync/driver/sync_token_status.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_request_status.h"

using bookmarks_helper::AddURL;

const char kShortLivedOAuth2Token[] =
    "{"
    "  \"refresh_token\": \"short_lived_refresh_token\","
    "  \"access_token\": \"short_lived_access_token\","
    "  \"expires_in\": 5,"  // 5 seconds.
    "  \"token_type\": \"Bearer\""
    "}";

const char kValidOAuth2Token[] = "{"
                                 "  \"refresh_token\": \"new_refresh_token\","
                                 "  \"access_token\": \"new_access_token\","
                                 "  \"expires_in\": 3600,"  // 1 hour.
                                 "  \"token_type\": \"Bearer\""
                                 "}";

const char kInvalidGrantOAuth2Token[] = "{"
                                        "  \"error\": \"invalid_grant\""
                                        "}";

const char kInvalidClientOAuth2Token[] = "{"
                                         "  \"error\": \"invalid_client\""
                                         "}";

const char kEmptyOAuth2Token[] = "";

const char kMalformedOAuth2Token[] = "{ \"foo\": ";

// Waits until local changes are committed or an auth error is encountered.
class TestForAuthError : public UpdatedProgressMarkerChecker {
 public:
  explicit TestForAuthError(browser_sync::ProfileSyncService* service);

  // StatusChangeChecker implementation.
  bool IsExitConditionSatisfied() override;
  std::string GetDebugMessage() const override;
};

TestForAuthError::TestForAuthError(browser_sync::ProfileSyncService* service)
    : UpdatedProgressMarkerChecker(service) {}

bool TestForAuthError::IsExitConditionSatisfied() {
  return (service()->GetSyncTokenStatus().last_get_token_error.state() !=
          GoogleServiceAuthError::NONE) ||
         UpdatedProgressMarkerChecker::IsExitConditionSatisfied();
}

std::string TestForAuthError::GetDebugMessage() const {
  return "Waiting for auth error";
}

class SyncAuthTest : public SyncTest {
 public:
  SyncAuthTest() : SyncTest(SINGLE_CLIENT), bookmark_index_(0) {}
  ~SyncAuthTest() override {}

  // Helper function that adds a bookmark and waits for either an auth error, or
  // for the bookmark to be committed.  Returns true if it detects an auth
  // error, false if the bookmark is committed successfully.
  bool AttemptToTriggerAuthError() {
    int bookmark_index = GetNextBookmarkIndex();
    std::string title = base::StringPrintf("Bookmark %d", bookmark_index);
    GURL url = GURL(base::StringPrintf("http://www.foo%d.com", bookmark_index));
    EXPECT_NE(nullptr, AddURL(0, title, url));

    // Run until the bookmark is committed or an auth error is encountered.
    TestForAuthError(GetSyncService(0)).Wait();

    GoogleServiceAuthError oauth_error =
        GetSyncService(0)->GetSyncTokenStatus().last_get_token_error;

    return oauth_error.state() != GoogleServiceAuthError::NONE;
  }

  void DisableTokenFetchRetries() {
    // If ProfileSyncService observes a transient error like SERVICE_UNAVAILABLE
    // or CONNECTION_FAILED, this means the OAuth2TokenService has given up
    // trying to reach Gaia. In practice, OA2TS retries a fixed number of times,
    // but the count is transparent to PSS.
    // Override the max retry count in TokenService so that we instantly trigger
    // the case where ProfileSyncService must pick up where OAuth2TokenService
    // left off (in terms of retries).
    ProfileOAuth2TokenServiceFactory::GetForProfile(GetProfile(0))->
        set_max_authorization_token_fetch_retries_for_testing(0);
  }


 private:
  int GetNextBookmarkIndex() {
    return bookmark_index_++;
  }

  int bookmark_index_;

  DISALLOW_COPY_AND_ASSIGN(SyncAuthTest);
};

// Verify that sync works with a valid OAuth2 token.
IN_PROC_BROWSER_TEST_F(SyncAuthTest, Sanity) {
  ASSERT_TRUE(SetupSync());
  GetFakeServer()->SetAuthenticated();
  DisableTokenFetchRetries();
  SetOAuth2TokenResponse(kValidOAuth2Token,
                         net::HTTP_OK,
                         net::URLRequestStatus::SUCCESS);
  ASSERT_FALSE(AttemptToTriggerAuthError());
}

// Verify that ProfileSyncService continues trying to fetch access tokens
// when OAuth2TokenService has encountered more than a fixed number of
// HTTP_INTERNAL_SERVER_ERROR (500) errors.
IN_PROC_BROWSER_TEST_F(SyncAuthTest, RetryOnInternalServerError500) {
  ASSERT_TRUE(SetupSync());
  ASSERT_FALSE(AttemptToTriggerAuthError());
  GetFakeServer()->SetUnauthenticated();
  DisableTokenFetchRetries();
  SetOAuth2TokenResponse(kValidOAuth2Token,
                         net::HTTP_INTERNAL_SERVER_ERROR,
                         net::URLRequestStatus::SUCCESS);
  ASSERT_TRUE(AttemptToTriggerAuthError());
  ASSERT_TRUE(GetSyncService(0)->IsRetryingAccessTokenFetchForTest());
}

// Verify that ProfileSyncService continues trying to fetch access tokens
// when OAuth2TokenService has encountered more than a fixed number of
// HTTP_FORBIDDEN (403) errors.
IN_PROC_BROWSER_TEST_F(SyncAuthTest, RetryOnHttpForbidden403) {
  ASSERT_TRUE(SetupSync());
  ASSERT_FALSE(AttemptToTriggerAuthError());
  GetFakeServer()->SetUnauthenticated();
  DisableTokenFetchRetries();
  SetOAuth2TokenResponse(kEmptyOAuth2Token,
                         net::HTTP_FORBIDDEN,
                         net::URLRequestStatus::SUCCESS);
  ASSERT_TRUE(AttemptToTriggerAuthError());
  ASSERT_TRUE(GetSyncService(0)->IsRetryingAccessTokenFetchForTest());
}

// Verify that ProfileSyncService continues trying to fetch access tokens
// when OAuth2TokenService has encountered a URLRequestStatus of FAILED.
IN_PROC_BROWSER_TEST_F(SyncAuthTest, RetryOnRequestFailed) {
  ASSERT_TRUE(SetupSync());
  ASSERT_FALSE(AttemptToTriggerAuthError());
  GetFakeServer()->SetUnauthenticated();
  DisableTokenFetchRetries();
  SetOAuth2TokenResponse(kEmptyOAuth2Token,
                         net::HTTP_INTERNAL_SERVER_ERROR,
                         net::URLRequestStatus::FAILED);
  ASSERT_TRUE(AttemptToTriggerAuthError());
  ASSERT_TRUE(GetSyncService(0)->IsRetryingAccessTokenFetchForTest());
}

// Verify that ProfileSyncService continues trying to fetch access tokens
// when OAuth2TokenService receives a malformed token.
IN_PROC_BROWSER_TEST_F(SyncAuthTest, RetryOnMalformedToken) {
  ASSERT_TRUE(SetupSync());
  ASSERT_FALSE(AttemptToTriggerAuthError());
  GetFakeServer()->SetUnauthenticated();
  DisableTokenFetchRetries();
  SetOAuth2TokenResponse(kMalformedOAuth2Token,
                         net::HTTP_OK,
                         net::URLRequestStatus::SUCCESS);
  ASSERT_TRUE(AttemptToTriggerAuthError());
  ASSERT_TRUE(GetSyncService(0)->IsRetryingAccessTokenFetchForTest());
}

// Verify that ProfileSyncService ends up with an INVALID_GAIA_CREDENTIALS auth
// error when an invalid_grant error is returned by OAuth2TokenService with an
// HTTP_BAD_REQUEST (400) response code.
IN_PROC_BROWSER_TEST_F(SyncAuthTest, InvalidGrant) {
  ASSERT_TRUE(SetupSync());
  ASSERT_FALSE(AttemptToTriggerAuthError());
  GetFakeServer()->SetUnauthenticated();
  DisableTokenFetchRetries();
  SetOAuth2TokenResponse(kInvalidGrantOAuth2Token,
                         net::HTTP_BAD_REQUEST,
                         net::URLRequestStatus::SUCCESS);
  ASSERT_TRUE(AttemptToTriggerAuthError());
  ASSERT_EQ(GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS,
            GetSyncService(0)->GetAuthError().state());
}

// Verify that ProfileSyncService retries after SERVICE_ERROR auth error when
// an invalid_client error is returned by OAuth2TokenService with an
// HTTP_BAD_REQUEST (400) response code.
IN_PROC_BROWSER_TEST_F(SyncAuthTest, RetryInvalidClient) {
  ASSERT_TRUE(SetupSync());
  ASSERT_FALSE(AttemptToTriggerAuthError());
  GetFakeServer()->SetUnauthenticated();
  DisableTokenFetchRetries();
  SetOAuth2TokenResponse(kInvalidClientOAuth2Token,
                         net::HTTP_BAD_REQUEST,
                         net::URLRequestStatus::SUCCESS);
  ASSERT_TRUE(AttemptToTriggerAuthError());
  ASSERT_TRUE(GetSyncService(0)->IsRetryingAccessTokenFetchForTest());
}

// Verify that ProfileSyncService retries after REQUEST_CANCELED auth error
// when OAuth2TokenService has encountered a URLRequestStatus of CANCELED.
IN_PROC_BROWSER_TEST_F(SyncAuthTest, RetryRequestCanceled) {
  ASSERT_TRUE(SetupSync());
  ASSERT_FALSE(AttemptToTriggerAuthError());
  GetFakeServer()->SetUnauthenticated();
  DisableTokenFetchRetries();
  SetOAuth2TokenResponse(kEmptyOAuth2Token,
                         net::HTTP_INTERNAL_SERVER_ERROR,
                         net::URLRequestStatus::CANCELED);
  ASSERT_TRUE(AttemptToTriggerAuthError());
  ASSERT_TRUE(GetSyncService(0)->IsRetryingAccessTokenFetchForTest());
}

// Verify that ProfileSyncService fails initial sync setup during backend
// initialization and ends up with an INVALID_GAIA_CREDENTIALS auth error when
// an invalid_grant error is returned by OAuth2TokenService with an
// HTTP_BAD_REQUEST (400) response code.
IN_PROC_BROWSER_TEST_F(SyncAuthTest, FailInitialSetupWithPersistentError) {
  ASSERT_TRUE(SetupClients());
  GetFakeServer()->SetUnauthenticated();
  DisableTokenFetchRetries();
  SetOAuth2TokenResponse(kInvalidGrantOAuth2Token,
                         net::HTTP_BAD_REQUEST,
                         net::URLRequestStatus::SUCCESS);
  ASSERT_FALSE(GetClient(0)->SetupSync());
  ASSERT_FALSE(GetSyncService(0)->IsSyncActive());
  ASSERT_EQ(GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS,
            GetSyncService(0)->GetAuthError().state());
}

// Verify that ProfileSyncService fails initial sync setup during backend
// initialization, but continues trying to fetch access tokens when
// OAuth2TokenService receives an HTTP_INTERNAL_SERVER_ERROR (500) response
// code.
IN_PROC_BROWSER_TEST_F(SyncAuthTest, RetryInitialSetupWithTransientError) {
  ASSERT_TRUE(SetupClients());
  GetFakeServer()->SetUnauthenticated();
  DisableTokenFetchRetries();
  SetOAuth2TokenResponse(kEmptyOAuth2Token,
                         net::HTTP_INTERNAL_SERVER_ERROR,
                         net::URLRequestStatus::SUCCESS);
  ASSERT_FALSE(GetClient(0)->SetupSync());
  ASSERT_FALSE(GetSyncService(0)->IsSyncActive());
  ASSERT_TRUE(GetSyncService(0)->IsRetryingAccessTokenFetchForTest());
}

// Verify that ProfileSyncService fetches a new token when an old token expires.
IN_PROC_BROWSER_TEST_F(SyncAuthTest, TokenExpiry) {
  // Initial sync succeeds with a short lived OAuth2 Token.
  ASSERT_TRUE(SetupClients());
  GetFakeServer()->SetAuthenticated();
  DisableTokenFetchRetries();
  SetOAuth2TokenResponse(kShortLivedOAuth2Token,
                         net::HTTP_OK,
                         net::URLRequestStatus::SUCCESS);
  ASSERT_TRUE(GetClient(0)->SetupSync());
  std::string old_token = GetSyncService(0)->GetAccessTokenForTest();

  // Wait until the token has expired.
  base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(5));

  // Trigger an auth error on the server so PSS requests OA2TS for a new token
  // during the next sync cycle.
  GetFakeServer()->SetUnauthenticated();
  SetOAuth2TokenResponse(kEmptyOAuth2Token,
                         net::HTTP_INTERNAL_SERVER_ERROR,
                         net::URLRequestStatus::SUCCESS);
  ASSERT_TRUE(AttemptToTriggerAuthError());
  ASSERT_TRUE(GetSyncService(0)->IsRetryingAccessTokenFetchForTest());

  // Trigger an auth success state and set up a new valid OAuth2 token.
  GetFakeServer()->SetAuthenticated();
  SetOAuth2TokenResponse(kValidOAuth2Token,
                         net::HTTP_OK,
                         net::URLRequestStatus::SUCCESS);

  // Verify that the next sync cycle is successful, and uses the new auth token.
  ASSERT_TRUE(UpdatedProgressMarkerChecker(GetSyncService(0)).Wait());
  std::string new_token = GetSyncService(0)->GetAccessTokenForTest();
  ASSERT_NE(old_token, new_token);
}
