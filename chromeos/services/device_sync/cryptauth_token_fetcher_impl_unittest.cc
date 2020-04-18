// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/device_sync/cryptauth_token_fetcher_impl.h"

#include "base/callback.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "services/identity/public/cpp/identity_test_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace device_sync {

namespace {

const char kAccessToken[] = "access_token";
const char kTestEmail[] = "example@gmail.com";

}  // namespace

class DeviceSyncCryptAuthAccessTokenFetcherImplTest : public testing::Test {
 protected:
  DeviceSyncCryptAuthAccessTokenFetcherImplTest() {}

  void SetUp() override {
    identity_test_environment_ =
        std::make_unique<identity::IdentityTestEnvironment>();
    identity_test_environment_->MakePrimaryAccountAvailable(kTestEmail);

    token_fetcher_ = std::make_unique<CryptAuthAccessTokenFetcherImpl>(
        identity_test_environment_->identity_manager());
  }

  void TearDown() override {
    // Deleting the object should not result in any additional tokens provided.
    token_fetcher_.reset();
    EXPECT_EQ(nullptr, GetTokenAndReset());

    EXPECT_TRUE(on_access_token_received_callback_.is_null());
  }

  void set_on_access_token_received_callback(
      base::OnceClosure on_access_token_received_callback) {
    EXPECT_TRUE(on_access_token_received_callback_.is_null());
    on_access_token_received_callback_ =
        std::move(on_access_token_received_callback);
  }

  void StartFetchingAccessToken() {
    token_fetcher_->FetchAccessToken(base::Bind(
        &DeviceSyncCryptAuthAccessTokenFetcherImplTest::OnAccessTokenFetched,
        base::Unretained(this)));
  }

  void OnAccessTokenFetched(const std::string& access_token) {
    last_access_token_ = std::make_unique<std::string>(access_token);
    if (!on_access_token_received_callback_.is_null())
      std::move(on_access_token_received_callback_).Run();
  }

  std::unique_ptr<std::string> GetTokenAndReset() {
    return std::move(last_access_token_);
  }

  const base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<std::string> last_access_token_;
  base::OnceClosure on_access_token_received_callback_;

  std::unique_ptr<identity::IdentityTestEnvironment> identity_test_environment_;
  std::unique_ptr<CryptAuthAccessTokenFetcherImpl> token_fetcher_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceSyncCryptAuthAccessTokenFetcherImplTest);
};

TEST_F(DeviceSyncCryptAuthAccessTokenFetcherImplTest, TestSuccess) {
  base::RunLoop run_loop;
  set_on_access_token_received_callback(run_loop.QuitClosure());

  // Start fetching the token. Nothing should be returned yet since the message
  // loop has not been run.
  StartFetchingAccessToken();
  EXPECT_EQ(nullptr, GetTokenAndReset());

  identity_test_environment_
      ->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
          kAccessToken, base::Time::Max() /* expiration */);

  // Run the request and confirm that the access token was returned.
  run_loop.Run();
  EXPECT_EQ(kAccessToken, *GetTokenAndReset());
}

TEST_F(DeviceSyncCryptAuthAccessTokenFetcherImplTest, TestFailure) {
  base::RunLoop run_loop;
  set_on_access_token_received_callback(run_loop.QuitClosure());

  // Start fetching the token. Nothing should be returned yet since the message
  // loop has not been run.
  StartFetchingAccessToken();
  EXPECT_EQ(nullptr, GetTokenAndReset());

  identity_test_environment_
      ->WaitForAccessTokenRequestIfNecessaryAndRespondWithError(
          GoogleServiceAuthError(
              GoogleServiceAuthError::State::INVALID_GAIA_CREDENTIALS));

  // Run the request and confirm that an empty string was returned, signifying a
  // failure to fetch the token.
  run_loop.Run();
  EXPECT_EQ(std::string(), *GetTokenAndReset());
}

TEST_F(DeviceSyncCryptAuthAccessTokenFetcherImplTest,
       TestDeletedBeforeOperationFinished) {
  StartFetchingAccessToken();
  EXPECT_EQ(nullptr, GetTokenAndReset());

  // Deleting the object should result in the callback being invoked with an
  // empty string (indicating failure).
  token_fetcher_.reset();
  EXPECT_EQ(std::string(), *GetTokenAndReset());
}

}  // namespace device_sync

}  // namespace chromeos
