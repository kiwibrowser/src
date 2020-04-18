// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/app_remoting_test_driver_environment.h"

#include <stddef.h>

#include <algorithm>
#include <utility>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "remoting/test/fake_access_token_fetcher.h"
#include "remoting/test/fake_app_remoting_report_issue_request.h"
#include "remoting/test/fake_refresh_token_store.h"
#include "remoting/test/fake_remote_host_info_fetcher.h"
#include "remoting/test/mock_access_token_fetcher.h"
#include "remoting/test/refresh_token_store.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const char kAuthCodeValue[] = "4/892379827345jkefvkdfbv";
const char kUserNameValue[] = "remoting_user@gmail.com";
const char kTestApplicationId[] = "sadlkjlsjgadjfgoajdfgagb";
const char kAnotherTestApplicationId[] = "waklgoisdhfnvjkdsfbljn";
const char kTestHostId1[] = "awesome_test_host_id";
const char kTestHostId2[] = "super_awesome_test_host_id";
const char kTestHostId3[] = "uber_awesome_test_host_id";
}

namespace remoting {
namespace test {

using testing::_;

class AppRemotingTestDriverEnvironmentTest : public ::testing::Test {
 public:
  AppRemotingTestDriverEnvironmentTest();
  ~AppRemotingTestDriverEnvironmentTest() override;

  FakeAccessTokenFetcher* fake_access_token_fetcher() const {
    return fake_access_token_fetcher_;
  }

 protected:
  void Initialize();
  void Initialize(
      const AppRemotingTestDriverEnvironment::EnvironmentOptions& options);

  FakeAccessTokenFetcher* fake_access_token_fetcher_;
  FakeAppRemotingReportIssueRequest fake_report_issue_request_;
  FakeRefreshTokenStore fake_token_store_;
  FakeRemoteHostInfoFetcher fake_remote_host_info_fetcher_;
  MockAccessTokenFetcher mock_access_token_fetcher_;

  std::unique_ptr<AppRemotingTestDriverEnvironment> environment_object_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AppRemotingTestDriverEnvironmentTest);
};

AppRemotingTestDriverEnvironmentTest::AppRemotingTestDriverEnvironmentTest()
    : fake_access_token_fetcher_(nullptr) {
}

AppRemotingTestDriverEnvironmentTest::~AppRemotingTestDriverEnvironmentTest() =
    default;

void AppRemotingTestDriverEnvironmentTest::Initialize() {
  AppRemotingTestDriverEnvironment::EnvironmentOptions options;
  options.user_name = kUserNameValue;
  options.service_environment = kDeveloperEnvironment;

  Initialize(options);
}

void AppRemotingTestDriverEnvironmentTest::Initialize(
    const AppRemotingTestDriverEnvironment::EnvironmentOptions& options) {
  environment_object_.reset(new AppRemotingTestDriverEnvironment(options));

  std::unique_ptr<FakeAccessTokenFetcher> fake_access_token_fetcher(
      new FakeAccessTokenFetcher());
  fake_access_token_fetcher_ = fake_access_token_fetcher.get();
  mock_access_token_fetcher_.SetAccessTokenFetcher(
      std::move(fake_access_token_fetcher));

  environment_object_->SetAccessTokenFetcherForTest(
      &mock_access_token_fetcher_);
  environment_object_->SetAppRemotingReportIssueRequestForTest(
      &fake_report_issue_request_);
  environment_object_->SetRefreshTokenStoreForTest(&fake_token_store_);
  environment_object_->SetRemoteHostInfoFetcherForTest(
      &fake_remote_host_info_fetcher_);
}

TEST_F(AppRemotingTestDriverEnvironmentTest, InitializeObjectWithAuthCode) {
  Initialize();

  EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromAuthCode(_, _));

  EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromRefreshToken(_, _))
      .Times(0);

  EXPECT_TRUE(environment_object_->Initialize(kAuthCodeValue));
  EXPECT_TRUE(fake_token_store_.refresh_token_write_attempted());
  EXPECT_EQ(fake_token_store_.stored_refresh_token_value(),
            kFakeAccessTokenFetcherRefreshTokenValue);
  EXPECT_EQ(environment_object_->user_name(), kUserNameValue);
  EXPECT_EQ(environment_object_->access_token(),
            kFakeAccessTokenFetcherAccessTokenValue);

  // Attempt to init again, we should not see any additional calls or errors.
  EXPECT_TRUE(environment_object_->Initialize(kAuthCodeValue));
}

TEST_F(AppRemotingTestDriverEnvironmentTest,
       InitializeObjectWithAuthCodeFailed) {
  Initialize();

  fake_access_token_fetcher()->set_fail_access_token_from_auth_code(true);

  EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromAuthCode(_, _));

  EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromRefreshToken(_, _))
      .Times(0);

  EXPECT_FALSE(environment_object_->Initialize(kAuthCodeValue));
  EXPECT_FALSE(fake_token_store_.refresh_token_write_attempted());
}

TEST_F(AppRemotingTestDriverEnvironmentTest, InitializeObjectWithRefreshToken) {
  Initialize();

  EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromRefreshToken(_, _));

  EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromAuthCode(_, _))
      .Times(0);

  // Pass in an empty auth code since we are using a refresh token.
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  // We should not write the refresh token a second time if we read from the
  // disk originally.
  EXPECT_FALSE(fake_token_store_.refresh_token_write_attempted());

  // Verify the object was initialized correctly.
  EXPECT_EQ(environment_object_->user_name(), kUserNameValue);
  EXPECT_EQ(environment_object_->access_token(),
            kFakeAccessTokenFetcherAccessTokenValue);

  // Attempt to init again, we should not see any additional calls or errors.
  EXPECT_TRUE(environment_object_->Initialize(std::string()));
}

TEST_F(AppRemotingTestDriverEnvironmentTest, TearDownAfterInitializeSucceeds) {
  Initialize();

  EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromRefreshToken(_, _));

  EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromAuthCode(_, _))
      .Times(0);

  // Pass in an empty auth code since we are using a refresh token.
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  // Note: We are using a static cast here because the TearDown() method is
  //       private as it is an interface method that we only want to call
  //       directly in tests or by the GTEST framework.
  static_cast<testing::Environment*>(environment_object_.get())->TearDown();
}

TEST_F(AppRemotingTestDriverEnvironmentTest,
       InitializeObjectWithRefreshTokenFailed) {
  Initialize();

  fake_access_token_fetcher()->set_fail_access_token_from_refresh_token(true);

  EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromRefreshToken(_, _));

  EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromAuthCode(_, _))
      .Times(0);

  // Pass in an empty auth code since we are using a refresh token.
  EXPECT_FALSE(environment_object_->Initialize(std::string()));
  EXPECT_FALSE(fake_token_store_.refresh_token_write_attempted());
}

TEST_F(AppRemotingTestDriverEnvironmentTest,
       InitializeObjectNoAuthCodeOrRefreshToken) {
  Initialize();

  // Neither method should be called in this scenario.
  EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromAuthCode(_, _))
      .Times(0);

  EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromRefreshToken(_, _))
      .Times(0);

  // Clear out the 'stored' refresh token value.
  fake_token_store_.set_refresh_token_value(std::string());

  // With no auth code or refresh token, then the initialization should fail.
  EXPECT_FALSE(environment_object_->Initialize(std::string()));
  EXPECT_FALSE(fake_token_store_.refresh_token_write_attempted());
}

TEST_F(AppRemotingTestDriverEnvironmentTest,
       InitializeObjectWithAuthCodeWriteFailed) {
  Initialize();

  EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromAuthCode(_, _));

  EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromRefreshToken(_, _))
      .Times(0);

  // Simulate a failure writing the token to the disk.
  fake_token_store_.set_refresh_token_write_succeeded(false);

  EXPECT_FALSE(environment_object_->Initialize(kAuthCodeValue));
  EXPECT_TRUE(fake_token_store_.refresh_token_write_attempted());
}

TEST_F(AppRemotingTestDriverEnvironmentTest,
       RefreshAccessTokenAfterUsingAuthCode) {
  Initialize();

  {
    testing::InSequence call_sequence;

    EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromAuthCode(_, _));

    EXPECT_CALL(mock_access_token_fetcher_,
                GetAccessTokenFromRefreshToken(_, _));
  }

  EXPECT_TRUE(environment_object_->Initialize(kAuthCodeValue));
  EXPECT_TRUE(fake_token_store_.refresh_token_write_attempted());
  EXPECT_EQ(fake_token_store_.stored_refresh_token_value(),
            kFakeAccessTokenFetcherRefreshTokenValue);
  EXPECT_EQ(environment_object_->user_name(), kUserNameValue);
  EXPECT_EQ(environment_object_->access_token(),
            kFakeAccessTokenFetcherAccessTokenValue);

  // Attempt to init again, we should not see any additional calls or errors.
  EXPECT_TRUE(environment_object_->RefreshAccessToken());
}

TEST_F(AppRemotingTestDriverEnvironmentTest, RefreshAccessTokenFailure) {
  Initialize();

  {
    testing::InSequence call_sequence;

    // Mock is set up for this call to succeed.
    EXPECT_CALL(mock_access_token_fetcher_, GetAccessTokenFromAuthCode(_, _));

    // Mock is set up for this call to fail.
    EXPECT_CALL(mock_access_token_fetcher_,
                GetAccessTokenFromRefreshToken(_, _));
  }

  EXPECT_TRUE(environment_object_->Initialize(kAuthCodeValue));
  EXPECT_TRUE(fake_token_store_.refresh_token_write_attempted());
  EXPECT_EQ(fake_token_store_.stored_refresh_token_value(),
            kFakeAccessTokenFetcherRefreshTokenValue);
  EXPECT_EQ(environment_object_->user_name(), kUserNameValue);
  EXPECT_EQ(environment_object_->access_token(),
            kFakeAccessTokenFetcherAccessTokenValue);

  fake_access_token_fetcher()->set_fail_access_token_from_refresh_token(true);

  // We expect the refresh to have failed, the user name to remain valid,
  // and the access token to have been cleared.
  EXPECT_FALSE(environment_object_->RefreshAccessToken());
  EXPECT_TRUE(environment_object_->access_token().empty());
  EXPECT_EQ(environment_object_->user_name(), kUserNameValue);
}

TEST_F(AppRemotingTestDriverEnvironmentTest, GetRemoteHostInfoSuccess) {
  Initialize();

  // Pass in an empty auth code since we are using a refresh token.
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  RemoteHostInfo remote_host_info;
  EXPECT_TRUE(environment_object_->GetRemoteHostInfoForApplicationId(
      kTestApplicationId, &remote_host_info));
  EXPECT_TRUE(remote_host_info.IsReadyForConnection());
}

TEST_F(AppRemotingTestDriverEnvironmentTest, GetRemoteHostInfoFailure) {
  Initialize();

  // Pass in an empty auth code since we are using a refresh token.
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  fake_remote_host_info_fetcher_.set_fail_retrieve_remote_host_info(true);

  RemoteHostInfo remote_host_info;
  EXPECT_FALSE(environment_object_->GetRemoteHostInfoForApplicationId(
      kTestApplicationId, &remote_host_info));
}

TEST_F(AppRemotingTestDriverEnvironmentTest,
       GetRemoteHostInfoWithoutInitializing) {
  Initialize();

  RemoteHostInfo remote_host_info;
  EXPECT_FALSE(environment_object_->GetRemoteHostInfoForApplicationId(
      kTestApplicationId, &remote_host_info));
}

TEST_F(AppRemotingTestDriverEnvironmentTest, NoRemoteHostsReleasedOnTearDown) {
  // Use the default options as the flag to release the remote hosts is not
  // enabled by default.
  Initialize();

  // Pass in an empty auth code since we are using a refresh token.
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  RemoteHostInfo remote_host_info;
  EXPECT_TRUE(environment_object_->GetRemoteHostInfoForApplicationId(
      kTestApplicationId, &remote_host_info));
  EXPECT_TRUE(remote_host_info.IsReadyForConnection());

  EXPECT_EQ(fake_report_issue_request_.get_host_ids_released().size(), 0UL);

  environment_object_->AddHostToReleaseList(kTestApplicationId, kTestHostId1);

  // Note: We are using a static cast here because the TearDown() method is
  //       private as it is an interface method that we only want to call
  //       directly in tests or by the GTEST framework.
  static_cast<testing::Environment*>(environment_object_.get())->TearDown();

  // Verify no hosts were released via a report issue request.
  EXPECT_EQ(fake_report_issue_request_.get_host_ids_released().size(), 0UL);
}

TEST_F(AppRemotingTestDriverEnvironmentTest, OneRemoteHostReleasedOnTearDown) {
  AppRemotingTestDriverEnvironment::EnvironmentOptions options;
  options.user_name = kUserNameValue;
  options.release_hosts_when_done = true;
  options.service_environment = kDeveloperEnvironment;
  Initialize(options);

  // Pass in an empty auth code since we are using a refresh token.
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  RemoteHostInfo remote_host_info;
  EXPECT_TRUE(environment_object_->GetRemoteHostInfoForApplicationId(
      kTestApplicationId, &remote_host_info));
  EXPECT_TRUE(remote_host_info.IsReadyForConnection());

  EXPECT_EQ(fake_report_issue_request_.get_host_ids_released().size(), 0UL);

  environment_object_->AddHostToReleaseList(kTestApplicationId, kTestHostId1);

  // Note: We are using a static cast here because the TearDown() method is
  //       private as it is an interface method that we only want to call
  //       directly in tests or by the GTEST framework.
  static_cast<testing::Environment*>(environment_object_.get())->TearDown();

  std::string expected_host(
      MakeFormattedStringForReleasedHost(kTestApplicationId, kTestHostId1));
  std::vector<std::string> actual_host_list =
      fake_report_issue_request_.get_host_ids_released();

  EXPECT_EQ(actual_host_list.size(), 1UL);
  EXPECT_EQ(actual_host_list[0], expected_host);
}

TEST_F(AppRemotingTestDriverEnvironmentTest, RemoteHostsReleasedOnTearDown) {
  AppRemotingTestDriverEnvironment::EnvironmentOptions options;
  options.user_name = kUserNameValue;
  options.release_hosts_when_done = true;
  options.service_environment = kDeveloperEnvironment;
  Initialize(options);

  // Pass in an empty auth code since we are using a refresh token.
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  RemoteHostInfo remote_host_info;
  EXPECT_TRUE(environment_object_->GetRemoteHostInfoForApplicationId(
      kTestApplicationId, &remote_host_info));
  EXPECT_TRUE(remote_host_info.IsReadyForConnection());

  std::vector<std::string> expected_host_list;
  environment_object_->AddHostToReleaseList(kTestApplicationId, kTestHostId1);
  expected_host_list.push_back(
      MakeFormattedStringForReleasedHost(kTestApplicationId, kTestHostId1));

  environment_object_->AddHostToReleaseList(kAnotherTestApplicationId,
                                            kTestHostId2);
  expected_host_list.push_back(MakeFormattedStringForReleasedHost(
      kAnotherTestApplicationId, kTestHostId2));

  environment_object_->AddHostToReleaseList(kTestApplicationId, kTestHostId3);
  expected_host_list.push_back(
      MakeFormattedStringForReleasedHost(kTestApplicationId, kTestHostId3));

  // Note: We are using a static cast here because the TearDown() method is
  //       private as it is an interface method that we only want to call
  //       directly in tests or by the GTEST framework.
  static_cast<testing::Environment*>(environment_object_.get())->TearDown();

  std::vector<std::string> actual_host_list =
      fake_report_issue_request_.get_host_ids_released();

  std::sort(actual_host_list.begin(), actual_host_list.end());
  std::sort(expected_host_list.begin(), expected_host_list.end());

  EXPECT_EQ(actual_host_list.size(), expected_host_list.size());
  for (size_t i = 0; i < actual_host_list.size(); ++i) {
    EXPECT_EQ(actual_host_list[i], expected_host_list[i]);
  }
}

TEST_F(AppRemotingTestDriverEnvironmentTest, RemoteHostsReleasedOnce) {
  AppRemotingTestDriverEnvironment::EnvironmentOptions options;
  options.user_name = kUserNameValue;
  options.release_hosts_when_done = true;
  options.service_environment = kDeveloperEnvironment;
  Initialize(options);

  // Pass in an empty auth code since we are using a refresh token.
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  RemoteHostInfo remote_host_info;
  EXPECT_TRUE(environment_object_->GetRemoteHostInfoForApplicationId(
      kTestApplicationId, &remote_host_info));
  EXPECT_TRUE(remote_host_info.IsReadyForConnection());

  std::vector<std::string> expected_host_list;
  environment_object_->AddHostToReleaseList(kTestApplicationId, kTestHostId1);
  expected_host_list.push_back(
      MakeFormattedStringForReleasedHost(kTestApplicationId, kTestHostId1));

  environment_object_->AddHostToReleaseList(kAnotherTestApplicationId,
                                            kTestHostId2);
  expected_host_list.push_back(MakeFormattedStringForReleasedHost(
      kAnotherTestApplicationId, kTestHostId2));

  environment_object_->AddHostToReleaseList(kTestApplicationId, kTestHostId3);
  expected_host_list.push_back(
      MakeFormattedStringForReleasedHost(kTestApplicationId, kTestHostId3));

  // Attempt to add the previous hosts again, they should not be added since
  // they will already exist in the list of hosts to release.
  environment_object_->AddHostToReleaseList(kTestApplicationId, kTestHostId1);
  environment_object_->AddHostToReleaseList(kAnotherTestApplicationId,
                                            kTestHostId2);
  environment_object_->AddHostToReleaseList(kTestApplicationId, kTestHostId3);

  // Note: We are using a static cast here because the TearDown() method is
  //       private as it is an interface method that we only want to call
  //       directly in tests or by the GTEST framework.
  static_cast<testing::Environment*>(environment_object_.get())->TearDown();

  std::vector<std::string> actual_host_list =
      fake_report_issue_request_.get_host_ids_released();

  std::sort(actual_host_list.begin(), actual_host_list.end());
  std::sort(expected_host_list.begin(), expected_host_list.end());

  EXPECT_EQ(actual_host_list.size(), expected_host_list.size());
  for (size_t i = 0; i < actual_host_list.size(); ++i) {
    EXPECT_EQ(actual_host_list[i], expected_host_list[i]);
  }
}

}  // namespace test
}  // namespace remoting
