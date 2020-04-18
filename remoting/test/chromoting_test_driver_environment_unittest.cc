// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/chromoting_test_driver_environment.h"

#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "remoting/test/fake_access_token_fetcher.h"
#include "remoting/test/fake_host_list_fetcher.h"
#include "remoting/test/fake_refresh_token_store.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const char kAuthCodeValue[] = "4/892379827345jkefvkdfbv";
const char kUserNameValue[] = "remoting_user@gmail.com";
const char kHostNameValue[] = "fake_host_name";
const char kFakeHostNameValue[] = "fake_host_name";
const char kFakeHostIdValue[] = "fake_host_id";
const char kFakeHostJidValue[] = "fake_host_jid";
const char kFakeHostOfflineReasonValue[] = "fake_offline_reason";
const char kFakeHostPublicKeyValue[] = "fake_public_key";
const char kFakeHostFirstTokenUrlValue[] = "token_url_1";
const char kFakeHostSecondTokenUrlValue[] = "token_url_2";
const char kFakeHostThirdTokenUrlValue[] = "token_url_3";
const unsigned int kExpectedHostListSize = 1;
}  // namespace

namespace remoting {
namespace test {

class ChromotingTestDriverEnvironmentTest : public ::testing::Test {
 public:
  ChromotingTestDriverEnvironmentTest();
  ~ChromotingTestDriverEnvironmentTest() override;

 protected:
  // testing::Test interface.
  void SetUp() override;

  // Helper method which has access to private method in class under test.
  bool RefreshHostList();

  HostInfo CreateFakeHostInfo();

  FakeAccessTokenFetcher fake_access_token_fetcher_;
  FakeRefreshTokenStore fake_token_store_;
  FakeHostListFetcher fake_host_list_fetcher_;

  std::unique_ptr<ChromotingTestDriverEnvironment> environment_object_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromotingTestDriverEnvironmentTest);
};

ChromotingTestDriverEnvironmentTest::ChromotingTestDriverEnvironmentTest() =
    default;

ChromotingTestDriverEnvironmentTest::~ChromotingTestDriverEnvironmentTest() =
    default;

void ChromotingTestDriverEnvironmentTest::SetUp() {
  ChromotingTestDriverEnvironment::EnvironmentOptions options;
  options.user_name = kUserNameValue;
  options.host_name = kHostNameValue;

  environment_object_.reset(new ChromotingTestDriverEnvironment(options));

  std::vector<HostInfo> fake_host_list;
  fake_host_list.push_back(CreateFakeHostInfo());

  fake_host_list_fetcher_.set_retrieved_host_list(fake_host_list);

  environment_object_->SetAccessTokenFetcherForTest(
      &fake_access_token_fetcher_);
  environment_object_->SetRefreshTokenStoreForTest(&fake_token_store_);
  environment_object_->SetHostListFetcherForTest(&fake_host_list_fetcher_);
}

bool ChromotingTestDriverEnvironmentTest::RefreshHostList() {
  return environment_object_->RefreshHostList() &&
         environment_object_->FindHostInHostList();
}

HostInfo ChromotingTestDriverEnvironmentTest::CreateFakeHostInfo() {
  HostInfo host_info;
  host_info.host_id = kFakeHostIdValue;
  host_info.host_jid = kFakeHostJidValue;
  host_info.host_name = kFakeHostNameValue;
  host_info.offline_reason = kFakeHostOfflineReasonValue;
  host_info.public_key = kFakeHostPublicKeyValue;
  host_info.status = kHostStatusOnline;
  host_info.token_url_patterns.push_back(kFakeHostFirstTokenUrlValue);
  host_info.token_url_patterns.push_back(kFakeHostSecondTokenUrlValue);
  host_info.token_url_patterns.push_back(kFakeHostThirdTokenUrlValue);

  return host_info;
}

TEST_F(ChromotingTestDriverEnvironmentTest, InitializeObjectWithAuthCode) {
  // Pass in an auth code to initialize the environment.
  EXPECT_TRUE(environment_object_->Initialize(kAuthCodeValue));

  EXPECT_TRUE(fake_token_store_.refresh_token_write_attempted());
  EXPECT_EQ(fake_token_store_.stored_refresh_token_value(),
            kFakeAccessTokenFetcherRefreshTokenValue);

  EXPECT_EQ(environment_object_->user_name(), kUserNameValue);
  EXPECT_EQ(environment_object_->host_name(), kHostNameValue);
  EXPECT_EQ(environment_object_->access_token(),
            kFakeAccessTokenFetcherAccessTokenValue);
  EXPECT_EQ(environment_object_->host_list().size(), 0u);

  // Now retrieve the host list.
  EXPECT_TRUE(environment_object_->WaitForHostOnline());

  // Should only have one host in the list.
  EXPECT_EQ(environment_object_->host_list().size(), kExpectedHostListSize);
  HostInfo fake_host = environment_object_->host_list().at(0);
  EXPECT_EQ(fake_host.host_id, kFakeHostIdValue);
  EXPECT_EQ(fake_host.host_jid, kFakeHostJidValue);
  EXPECT_EQ(fake_host.host_name, kFakeHostNameValue);
  EXPECT_EQ(fake_host.public_key, kFakeHostPublicKeyValue);
  EXPECT_EQ(fake_host.token_url_patterns.at(0), kFakeHostFirstTokenUrlValue);
  EXPECT_EQ(fake_host.token_url_patterns.at(1), kFakeHostSecondTokenUrlValue);
  EXPECT_EQ(fake_host.token_url_patterns.at(2), kFakeHostThirdTokenUrlValue);
}

TEST_F(ChromotingTestDriverEnvironmentTest,
       InitializeObjectWithAuthCodeFailed) {
  fake_access_token_fetcher_.set_fail_access_token_from_auth_code(true);

  EXPECT_FALSE(environment_object_->Initialize(kAuthCodeValue));
  EXPECT_FALSE(fake_token_store_.refresh_token_write_attempted());
}

TEST_F(ChromotingTestDriverEnvironmentTest, InitializeObjectWithRefreshToken) {
  // Pass in an empty auth code since we are using a refresh token.
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  // We should not write the refresh token a second time if we read from the
  // disk originally.
  EXPECT_FALSE(fake_token_store_.refresh_token_write_attempted());

  // Verify the object was initialized correctly.
  EXPECT_EQ(environment_object_->user_name(), kUserNameValue);
  EXPECT_EQ(environment_object_->host_name(), kHostNameValue);
  EXPECT_EQ(environment_object_->access_token(),
            kFakeAccessTokenFetcherAccessTokenValue);
  EXPECT_EQ(environment_object_->host_list().size(), 0u);

  // Now retrieve the host list.
  EXPECT_TRUE(environment_object_->WaitForHostOnline());

  // Should only have one host in the list.
  EXPECT_EQ(environment_object_->host_list().size(), kExpectedHostListSize);

  HostInfo fake_host = environment_object_->host_list().at(0);
  EXPECT_EQ(fake_host.host_id, kFakeHostIdValue);
  EXPECT_EQ(fake_host.host_jid, kFakeHostJidValue);
  EXPECT_EQ(fake_host.host_name, kFakeHostNameValue);
  EXPECT_EQ(fake_host.public_key, kFakeHostPublicKeyValue);
  EXPECT_EQ(fake_host.token_url_patterns.at(0), kFakeHostFirstTokenUrlValue);
  EXPECT_EQ(fake_host.token_url_patterns.at(1), kFakeHostSecondTokenUrlValue);
  EXPECT_EQ(fake_host.token_url_patterns.at(2), kFakeHostThirdTokenUrlValue);
}

TEST_F(ChromotingTestDriverEnvironmentTest,
       InitializeObjectWithRefreshTokenFailed) {
  fake_access_token_fetcher_.set_fail_access_token_from_refresh_token(true);

  // Pass in an empty auth code since we are using a refresh token.
  EXPECT_FALSE(environment_object_->Initialize(std::string()));
  EXPECT_FALSE(fake_token_store_.refresh_token_write_attempted());
}

TEST_F(ChromotingTestDriverEnvironmentTest, TearDownAfterInitializeSucceeds) {
  // Pass in an empty auth code since we are using a refresh token.
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  // Note: We are using a static cast here because the TearDown() method is
  //       private as it is an interface method that we only want to call
  //       directly in tests or by the GTEST framework.
  static_cast<testing::Environment*>(environment_object_.get())->TearDown();
}

TEST_F(ChromotingTestDriverEnvironmentTest,
       InitializeObjectNoAuthCodeOrRefreshToken) {
  // Clear out the 'stored' refresh token value.
  fake_token_store_.set_refresh_token_value(std::string());

  // With no auth code or refresh token, then the initialization should fail.
  EXPECT_FALSE(environment_object_->Initialize(std::string()));
  EXPECT_FALSE(fake_token_store_.refresh_token_write_attempted());
}

TEST_F(ChromotingTestDriverEnvironmentTest,
       InitializeObjectWithAuthCodeWriteFailed) {
  // Simulate a failure writing the token to the disk.
  fake_token_store_.set_refresh_token_write_succeeded(false);

  EXPECT_FALSE(environment_object_->Initialize(kAuthCodeValue));
  EXPECT_TRUE(fake_token_store_.refresh_token_write_attempted());
}

TEST_F(ChromotingTestDriverEnvironmentTest, HostListEmptyFromDirectory) {
  // Set the host list fetcher to return an empty list.
  fake_host_list_fetcher_.set_retrieved_host_list(std::vector<HostInfo>());

  EXPECT_TRUE(environment_object_->Initialize(kAuthCodeValue));
  EXPECT_TRUE(fake_token_store_.refresh_token_write_attempted());
}

TEST_F(ChromotingTestDriverEnvironmentTest, RefreshHostList_HostOnline) {
  EXPECT_TRUE(environment_object_->Initialize(std::string()));
  environment_object_->SetHostNameForTest(kFakeHostNameValue);
  environment_object_->SetHostJidForTest(kFakeHostJidValue);
  EXPECT_TRUE(RefreshHostList());
  EXPECT_TRUE(environment_object_->host_info().IsReadyForConnection());
}

TEST_F(ChromotingTestDriverEnvironmentTest,
       RefreshHostList_HostOnline_NoJidPassed) {
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  environment_object_->SetHostNameForTest(kFakeHostNameValue);
  environment_object_->SetHostJidForTest(std::string());
  EXPECT_TRUE(RefreshHostList());
  EXPECT_TRUE(environment_object_->host_info().IsReadyForConnection());
}

TEST_F(ChromotingTestDriverEnvironmentTest, RefreshHostList_NameMismatch) {
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  environment_object_->SetHostNameForTest("Ficticious_host");
  environment_object_->SetHostJidForTest(kFakeHostJidValue);
  EXPECT_FALSE(RefreshHostList());
  EXPECT_FALSE(environment_object_->host_info().IsReadyForConnection());
}

TEST_F(ChromotingTestDriverEnvironmentTest, RefreshHostList_JidMismatch) {
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  environment_object_->SetHostNameForTest(kFakeHostNameValue);
  environment_object_->SetHostJidForTest("Ficticious_jid");
  EXPECT_FALSE(RefreshHostList());
  EXPECT_FALSE(environment_object_->host_info().IsReadyForConnection());
}

TEST_F(ChromotingTestDriverEnvironmentTest, RefreshHostList_HostOffline) {
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  HostInfo fake_host(CreateFakeHostInfo());
  fake_host.status = kHostStatusOffline;

  std::vector<HostInfo> fake_host_list(1, fake_host);
  fake_host_list_fetcher_.set_retrieved_host_list(fake_host_list);

  environment_object_->SetHostNameForTest(kFakeHostNameValue);
  environment_object_->SetHostJidForTest(kFakeHostJidValue);
  EXPECT_TRUE(RefreshHostList());
  EXPECT_FALSE(environment_object_->host_info().IsReadyForConnection());
}

TEST_F(ChromotingTestDriverEnvironmentTest, RefreshHostList_HostListEmpty) {
  EXPECT_TRUE(environment_object_->Initialize(std::string()));

  // Set the host list fetcher to return an empty list.
  fake_host_list_fetcher_.set_retrieved_host_list(std::vector<HostInfo>());
  environment_object_->SetHostNameForTest(kFakeHostNameValue);
  environment_object_->SetHostJidForTest(kFakeHostJidValue);
  EXPECT_FALSE(RefreshHostList());
  EXPECT_FALSE(environment_object_->host_info().IsReadyForConnection());
}

}  // namespace test
}  // namespace remoting
