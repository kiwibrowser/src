// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#include "remoting/ios/facade/host_list_service.h"

#import <Foundation/Foundation.h>

#import "remoting/ios/facade/remoting_authentication.h"
#import "remoting/ios/facade/remoting_service.h"
#import "third_party/ocmock/OCMock/OCMock.h"

#include "base/bind.h"
#include "net/http/http_status_code.h"
#include "remoting/ios/facade/fake_host_list_fetcher.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

#define EXPECT_HOST_LIST_STATE(expected) \
  EXPECT_EQ(expected, host_list_service_->state())

#define EXPECT_NO_FETCH_FAILURE() \
  EXPECT_TRUE(host_list_service_->last_fetch_failure() == nullptr)

namespace remoting {

class HostListServiceTest : public PlatformTest {
 public:
  void SetUp() override;
  void TearDown() override;

 protected:
  void ExpectAuthResult(RemotingAuthenticationStatus status);
  void NotifyUserUpdate(bool is_signed_in);

  std::unique_ptr<HostListService> host_list_service_;
  FakeHostListFetcher* host_list_fetcher_;

  HostInfo fake_host_;

  int on_fetch_failed_call_count_ = 0;
  int on_host_list_state_changed_call_count_ = 0;

  id remoting_authentication_mock_;
  id remoting_service_mock_;

 private:
  std::unique_ptr<HostListService::CallbackSubscription>
      host_list_state_subscription_;
  std::unique_ptr<HostListService::CallbackSubscription>
      fetch_failure_subscription_;
};

void HostListServiceTest::SetUp() {
  static const char use_cocoa_locale[] = "";

  l10n_util::OverrideLocaleWithCocoaLocale();
  ui::ResourceBundle::InitSharedInstanceWithLocale(
      use_cocoa_locale, NULL, ui::ResourceBundle::DO_NOT_LOAD_COMMON_RESOURCES);

  on_fetch_failed_call_count_ = 0;
  on_host_list_state_changed_call_count_ = 0;

  remoting_authentication_mock_ =
      OCMProtocolMock(@protocol(RemotingAuthentication));

  remoting_service_mock_ = OCMClassMock([RemotingService class]);
  OCMStub([remoting_service_mock_ instance]).andReturn(remoting_service_mock_);
  OCMStub([remoting_service_mock_ authentication])
      .andReturn(remoting_authentication_mock_);

  host_list_service_ = HostListService::CreateInstanceForTesting();
  auto host_list_fetcher = std::make_unique<FakeHostListFetcher>();
  host_list_fetcher_ = host_list_fetcher.get();
  host_list_service_->SetHostListFetcherForTesting(
      std::move(host_list_fetcher));

  host_list_state_subscription_ =
      host_list_service_->RegisterHostListStateCallback(base::BindRepeating(
          [](HostListServiceTest* that) {
            that->on_host_list_state_changed_call_count_++;
          },
          base::Unretained(this)));
  fetch_failure_subscription_ =
      host_list_service_->RegisterFetchFailureCallback(base::BindRepeating(
          [](HostListServiceTest* that) {
            that->on_fetch_failed_call_count_++;
          },
          base::Unretained(this)));

  fake_host_.host_id = "fake_host_id";
}

void HostListServiceTest::TearDown() {
  ui::ResourceBundle::CleanupSharedInstance();
}

void HostListServiceTest::ExpectAuthResult(
    RemotingAuthenticationStatus status) {
  NSString* user_email =
      status == RemotingAuthenticationStatusSuccess ? @"fake@gmail.com" : nil;
  NSString* access_token =
      status == RemotingAuthenticationStatusSuccess ? @"fake_token" : nil;
  OCMStub([remoting_authentication_mock_ callbackWithAccessToken:[OCMArg any]])
      .andDo(^(NSInvocation* invocation) {
        AccessTokenCallback callback;
        [invocation getArgument:&callback atIndex:2];
        DCHECK(callback);
        callback(status, user_email, access_token);
      });
}

void HostListServiceTest::NotifyUserUpdate(bool is_signed_in) {
  NSDictionary* user_info =
      is_signed_in ? @{kUserInfo : [[UserInfo alloc] init]} : @{};
  [NSNotificationCenter.defaultCenter postNotificationName:kUserDidUpdate
                                                    object:nil
                                                  userInfo:user_info];
}

TEST_F(HostListServiceTest, SuccessfullyFetchHostList) {
  EXPECT_HOST_LIST_STATE(HostListService::State::NOT_FETCHED);
  EXPECT_NO_FETCH_FAILURE();

  ExpectAuthResult(RemotingAuthenticationStatusSuccess);
  host_list_service_->RequestFetch();
  EXPECT_HOST_LIST_STATE(HostListService::State::FETCHING);
  EXPECT_NO_FETCH_FAILURE();

  host_list_fetcher_->ResolveCallback(net::HTTP_OK, {fake_host_});
  EXPECT_HOST_LIST_STATE(HostListService::State::FETCHED);
  EXPECT_NO_FETCH_FAILURE();

  EXPECT_EQ(1u, host_list_service_->hosts().size());
  EXPECT_EQ(fake_host_.host_id, host_list_service_->hosts()[0].host_id);

  EXPECT_EQ(2, on_host_list_state_changed_call_count_);
  EXPECT_EQ(0, on_fetch_failed_call_count_);
}

TEST_F(HostListServiceTest, FetchHostListAuthFailed) {
  EXPECT_HOST_LIST_STATE(HostListService::State::NOT_FETCHED);
  EXPECT_NO_FETCH_FAILURE();

  ExpectAuthResult(RemotingAuthenticationStatusAuthError);
  host_list_service_->RequestFetch();
  EXPECT_HOST_LIST_STATE(HostListService::State::NOT_FETCHED);
  EXPECT_EQ(HostListService::FetchFailureReason::AUTH_ERROR,
            host_list_service_->last_fetch_failure()->reason);
  EXPECT_TRUE(host_list_service_->hosts().empty());

  EXPECT_EQ(0, on_host_list_state_changed_call_count_);
  EXPECT_EQ(1, on_fetch_failed_call_count_);
}

TEST_F(HostListServiceTest, FetchHostListRequestFailed) {
  EXPECT_HOST_LIST_STATE(HostListService::State::NOT_FETCHED);
  EXPECT_NO_FETCH_FAILURE();

  ExpectAuthResult(RemotingAuthenticationStatusSuccess);
  host_list_service_->RequestFetch();
  EXPECT_HOST_LIST_STATE(HostListService::State::FETCHING);
  EXPECT_NO_FETCH_FAILURE();

  host_list_fetcher_->ResolveCallback(net::HTTP_INTERNAL_SERVER_ERROR, {});
  EXPECT_HOST_LIST_STATE(HostListService::State::NOT_FETCHED);
  EXPECT_EQ(HostListService::FetchFailureReason::REQUEST_ERROR,
            host_list_service_->last_fetch_failure()->reason);
  EXPECT_EQ(net::HTTP_INTERNAL_SERVER_ERROR,
            host_list_service_->last_fetch_failure()->error_code);
  EXPECT_TRUE(host_list_service_->hosts().empty());

  EXPECT_EQ(2, on_host_list_state_changed_call_count_);
  EXPECT_EQ(1, on_fetch_failed_call_count_);
}

TEST_F(HostListServiceTest, FetchHostListRequestUnauthenticated_signOut) {
  EXPECT_HOST_LIST_STATE(HostListService::State::NOT_FETCHED);
  EXPECT_NO_FETCH_FAILURE();

  ExpectAuthResult(RemotingAuthenticationStatusSuccess);
  host_list_service_->RequestFetch();
  EXPECT_HOST_LIST_STATE(HostListService::State::FETCHING);
  EXPECT_NO_FETCH_FAILURE();

  OCMExpect([remoting_authentication_mock_ logout]);
  host_list_fetcher_->ResolveCallback(net::HTTP_UNAUTHORIZED, {});
  EXPECT_HOST_LIST_STATE(HostListService::State::NOT_FETCHED);
  [remoting_authentication_mock_ verifyAtLocation:nil];

  EXPECT_EQ(2, on_host_list_state_changed_call_count_);
  EXPECT_EQ(0, on_fetch_failed_call_count_);
}

TEST_F(HostListServiceTest, RequestFetchWhileFetching_ignoreSecondRequest) {
  EXPECT_HOST_LIST_STATE(HostListService::State::NOT_FETCHED);

  ExpectAuthResult(RemotingAuthenticationStatusSuccess);
  host_list_service_->RequestFetch();
  EXPECT_HOST_LIST_STATE(HostListService::State::FETCHING);

  host_list_service_->RequestFetch();
  EXPECT_HOST_LIST_STATE(HostListService::State::FETCHING);

  EXPECT_EQ(1, on_host_list_state_changed_call_count_);
  EXPECT_EQ(0, on_fetch_failed_call_count_);
}

TEST_F(HostListServiceTest, UserLogOut_cancelFetch) {
  EXPECT_HOST_LIST_STATE(HostListService::State::NOT_FETCHED);

  ExpectAuthResult(RemotingAuthenticationStatusSuccess);
  host_list_service_->RequestFetch();
  EXPECT_HOST_LIST_STATE(HostListService::State::FETCHING);

  host_list_fetcher_->ExpectCancelFetch();
  NotifyUserUpdate(false);
  EXPECT_HOST_LIST_STATE(HostListService::State::NOT_FETCHED);

  EXPECT_EQ(2, on_host_list_state_changed_call_count_);
  EXPECT_EQ(0, on_fetch_failed_call_count_);
}

TEST_F(HostListServiceTest, UserSwitchAccount_cancelThenRequestNewFetch) {
  EXPECT_HOST_LIST_STATE(HostListService::State::NOT_FETCHED);

  ExpectAuthResult(RemotingAuthenticationStatusSuccess);
  host_list_service_->RequestFetch();
  EXPECT_HOST_LIST_STATE(HostListService::State::FETCHING);

  host_list_fetcher_->ExpectCancelFetch();
  ExpectAuthResult(RemotingAuthenticationStatusSuccess);
  NotifyUserUpdate(true);
  EXPECT_HOST_LIST_STATE(HostListService::State::FETCHING);

  host_list_fetcher_->ResolveCallback(net::HTTP_OK, {fake_host_});
  EXPECT_HOST_LIST_STATE(HostListService::State::FETCHED);

  EXPECT_EQ(1u, host_list_service_->hosts().size());
  EXPECT_EQ(fake_host_.host_id, host_list_service_->hosts()[0].host_id);

  // Note that there is an extra FETCHING->NOT_FETCH change during
  // NotifyUserUpdate(true).
  EXPECT_EQ(4, on_host_list_state_changed_call_count_);
  EXPECT_EQ(0, on_fetch_failed_call_count_);
}

}  // namespace remoting
