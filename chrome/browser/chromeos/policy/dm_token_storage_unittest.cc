// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/dm_token_storage.h"

#include <memory>

#include "base/bind.h"
#include "base/run_loop.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chromeos/cryptohome/system_salt_getter.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_cryptohome_client.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

class DMTokenStorageTest : public testing::Test {
 public:
  DMTokenStorageTest()
      : scoped_testing_local_state_(TestingBrowserProcess::GetGlobal()) {}
  ~DMTokenStorageTest() override {}

  void SetSaltPending() {
    // Clear the cached salt.
    chromeos::SystemSaltGetter::Shutdown();
    fake_cryptohome_client_->set_system_salt(std::vector<uint8_t>());
    fake_cryptohome_client_->SetServiceIsAvailable(false);
    chromeos::SystemSaltGetter::Initialize();
  }

  void SetSaltAvailable() {
    fake_cryptohome_client_->set_system_salt(
        chromeos::FakeCryptohomeClient::GetStubSystemSalt());
    fake_cryptohome_client_->SetServiceIsAvailable(true);
  }

  void SetSaltError() {
    fake_cryptohome_client_->set_system_salt(std::vector<uint8_t>());
    fake_cryptohome_client_->SetServiceIsAvailable(true);
  }

  void SetUp() override {
    fake_cryptohome_client_ = new chromeos::FakeCryptohomeClient;
    SetSaltAvailable();
    chromeos::DBusThreadManager::GetSetterForTesting()->SetCryptohomeClient(
        std::unique_ptr<chromeos::CryptohomeClient>(fake_cryptohome_client_));

    chromeos::SystemSaltGetter::Initialize();
  }

  void TearDown() override {
    dm_token_storage_.reset();
    chromeos::SystemSaltGetter::Shutdown();
    chromeos::DBusThreadManager::Shutdown();
    base::RunLoop().RunUntilIdle();
  }

  void CreateDMStorage() {
    dm_token_storage_ =
        std::make_unique<DMTokenStorage>(scoped_testing_local_state_.Get());
  }

  void StoreDMToken() {
    base::RunLoop run_loop;
    dm_token_storage_->StoreDMToken(
        "test-token",
        base::BindOnce(&DMTokenStorageTest::OnStoreCallback,
                       base::Unretained(this), run_loop.QuitClosure(), true));
    run_loop.Run();
  }

  void OnStoreCallback(const base::Closure& closure,
                       bool expected,
                       bool success) {
    EXPECT_EQ(expected, success);
    if (!closure.is_null())
      closure.Run();
  }

  void OnRetrieveCallback(const base::Closure& closure,
                          const std::string& expected,
                          const std::string& actual) {
    EXPECT_EQ(expected, actual);
    if (!closure.is_null())
      closure.Run();
  }

  content::TestBrowserThreadBundle thread_bundle_;
  ScopedTestingLocalState scoped_testing_local_state_;
  chromeos::FakeCryptohomeClient* fake_cryptohome_client_;
  std::unique_ptr<DMTokenStorage> dm_token_storage_;
};

TEST_F(DMTokenStorageTest, SaveEncryptedToken) {
  CreateDMStorage();
  StoreDMToken();

  {
    base::RunLoop run_loop;
    dm_token_storage_->RetrieveDMToken(base::BindOnce(
        &DMTokenStorageTest::OnRetrieveCallback, base::Unretained(this),
        run_loop.QuitClosure(), "test-token"));
    run_loop.Run();
  }
  // Reload shouldn't change the token.
  CreateDMStorage();
  {
    base::RunLoop run_loop;
    dm_token_storage_->RetrieveDMToken(base::BindOnce(
        &DMTokenStorageTest::OnRetrieveCallback, base::Unretained(this),
        run_loop.QuitClosure(), "test-token"));
    run_loop.Run();
  }
  {
    // Subsequent retrieving DM token should succeed.
    base::RunLoop run_loop;
    dm_token_storage_->RetrieveDMToken(base::BindOnce(
        &DMTokenStorageTest::OnRetrieveCallback, base::Unretained(this),
        run_loop.QuitClosure(), "test-token"));
    run_loop.Run();
  }
}

TEST_F(DMTokenStorageTest, RetrieveEncryptedTokenWithPendingSalt) {
  CreateDMStorage();
  StoreDMToken();

  SetSaltPending();
  CreateDMStorage();

  {
    base::RunLoop run_loop;
    dm_token_storage_->RetrieveDMToken(base::BindOnce(
        &DMTokenStorageTest::OnRetrieveCallback, base::Unretained(this),
        run_loop.QuitClosure(), "test-token"));
    SetSaltAvailable();
    run_loop.Run();
  }
}

TEST_F(DMTokenStorageTest, StoreEncryptedTokenWithPendingSalt) {
  SetSaltPending();
  CreateDMStorage();
  base::RunLoop run_loop;
  dm_token_storage_->StoreDMToken(
      "test-token",
      base::BindOnce(&DMTokenStorageTest::OnStoreCallback,
                     base::Unretained(this), run_loop.QuitClosure(), true));
  SetSaltAvailable();
  run_loop.Run();
}

TEST_F(DMTokenStorageTest, MultipleRetrieveTokenCalls) {
  CreateDMStorage();
  StoreDMToken();
  {
    base::RunLoop run_loop;
    for (int i = 0; i < 3; ++i) {
      dm_token_storage_->RetrieveDMToken(base::BindOnce(
          &DMTokenStorageTest::OnRetrieveCallback, base::Unretained(this),
          run_loop.QuitClosure(), "test-token"));
    }
    run_loop.Run();
  }
}

TEST_F(DMTokenStorageTest, StoreWithSaltError) {
  SetSaltError();
  CreateDMStorage();
  base::RunLoop run_loop;
  dm_token_storage_->StoreDMToken(
      "test-token",
      base::BindOnce(&DMTokenStorageTest::OnStoreCallback,
                     base::Unretained(this), run_loop.QuitClosure(), false));
  run_loop.Run();
}

TEST_F(DMTokenStorageTest, RetrieveWithSaltError) {
  CreateDMStorage();
  StoreDMToken();
  SetSaltPending();
  CreateDMStorage();
  base::RunLoop run_loop;
  dm_token_storage_->RetrieveDMToken(
      base::BindOnce(&DMTokenStorageTest::OnRetrieveCallback,
                     base::Unretained(this), run_loop.QuitClosure(), ""));
  SetSaltError();
  run_loop.Run();
}

TEST_F(DMTokenStorageTest, RetrieveWithNoToken) {
  CreateDMStorage();
  base::RunLoop run_loop;
  dm_token_storage_->RetrieveDMToken(
      base::BindOnce(&DMTokenStorageTest::OnRetrieveCallback,
                     base::Unretained(this), run_loop.QuitClosure(), ""));
  run_loop.Run();
}

TEST_F(DMTokenStorageTest, RetrieveFailIfStoreRunning) {
  SetSaltPending();
  CreateDMStorage();
  base::RunLoop run_loop;
  dm_token_storage_->StoreDMToken(
      "test-token",
      base::BindOnce(&DMTokenStorageTest::OnStoreCallback,
                     base::Unretained(this), run_loop.QuitClosure(), true));
  dm_token_storage_->RetrieveDMToken(
      base::BindOnce(&DMTokenStorageTest::OnRetrieveCallback,
                     base::Unretained(this), base::Closure(), ""));
  SetSaltAvailable();
  run_loop.Run();
}

TEST_F(DMTokenStorageTest, StoreFailIfAnotherStoreRunning) {
  SetSaltPending();
  CreateDMStorage();
  base::RunLoop run_loop;
  dm_token_storage_->StoreDMToken(
      "test-token",
      base::BindOnce(&DMTokenStorageTest::OnStoreCallback,
                     base::Unretained(this), run_loop.QuitClosure(), true));
  dm_token_storage_->StoreDMToken(
      "test-token",
      base::BindOnce(&DMTokenStorageTest::OnStoreCallback,
                     base::Unretained(this), base::Closure(), false));
  SetSaltAvailable();
  run_loop.Run();
}

}  // namespace policy
