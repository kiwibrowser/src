// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/browser_dm_token_storage_win.h"

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_reg_util_win.h"
#include "base/win/registry.h"
#include "chrome/installer/util/install_util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::IsEmpty;

namespace policy {

namespace {

constexpr wchar_t kClientId1[] = L"fake-client-id-1";
constexpr wchar_t kClientId2[] = L"fake-client-id-2";
constexpr wchar_t kEnrollmentToken1[] = L"fake-enrollment-token-1";
constexpr wchar_t kEnrollmentToken2[] = L"fake-enrollment-token-2";
constexpr char kDMToken1[] = "fake-dm-token-1";
constexpr char kDMToken2[] = "fake-dm-token-2";

}  // namespace

class BrowserDMTokenStorageWinTest : public testing::Test {
 protected:
  BrowserDMTokenStorageWinTest() {}
  ~BrowserDMTokenStorageWinTest() override {}

  void SetUp() override {
    ASSERT_NO_FATAL_FAILURE(
        registry_override_manager_.OverrideRegistry(HKEY_LOCAL_MACHINE));
  }

  bool SetMachineGuid(const wchar_t* machine_guid) {
    base::win::RegKey key;
    return (key.Create(HKEY_LOCAL_MACHINE, L"software\\microsoft\\cryptography",
                       KEY_SET_VALUE) == ERROR_SUCCESS) &&
           (key.WriteValue(L"MachineGuid", machine_guid) == ERROR_SUCCESS);
  }

  bool SetEnrollmentToken(const wchar_t* enrollment_token) {
    base::string16 key_path;
    base::string16 value_name;
    InstallUtil::GetMachineLevelUserCloudPolicyEnrollmentTokenRegistryPath(
        &key_path, &value_name);
    base::win::RegKey key;
    return (key.Create(HKEY_LOCAL_MACHINE, key_path.c_str(), KEY_SET_VALUE) ==
            ERROR_SUCCESS) &&
           (key.WriteValue(value_name.c_str(), enrollment_token) ==
            ERROR_SUCCESS);
  }

  bool SetDMToken(const std::string& dm_token) {
    base::win::RegKey key;
    base::string16 dm_token_key_path;
    base::string16 dm_token_value_name;
    InstallUtil::GetMachineLevelUserCloudPolicyDMTokenRegistryPath(
        &dm_token_key_path, &dm_token_value_name);
    return (key.Create(HKEY_LOCAL_MACHINE, dm_token_key_path.c_str(),
                       KEY_SET_VALUE | KEY_WOW64_64KEY) == ERROR_SUCCESS) &&
           (key.WriteValue(dm_token_value_name.c_str(), dm_token.c_str(),
                           dm_token.size(), REG_BINARY) == ERROR_SUCCESS);
  }

  content::TestBrowserThreadBundle thread_bundle_;
  registry_util::RegistryOverrideManager registry_override_manager_;
};

TEST_F(BrowserDMTokenStorageWinTest, RetrieveClientId) {
  ASSERT_TRUE(SetMachineGuid(kClientId1));
  BrowserDMTokenStorageWin storage;
  EXPECT_EQ(base::WideToUTF8(kClientId1), storage.RetrieveClientId());

  // The client ID value should be cached in memory and not read from the system
  // again.
  ASSERT_TRUE(SetMachineGuid(kClientId2));
  EXPECT_EQ(base::WideToUTF8(kClientId1), storage.RetrieveClientId());
}

TEST_F(BrowserDMTokenStorageWinTest, RetrieveEnrollmentToken) {
  ASSERT_TRUE(SetMachineGuid(kClientId1));
  ASSERT_TRUE(SetEnrollmentToken(kEnrollmentToken1));

  BrowserDMTokenStorageWin storage;
  EXPECT_EQ(base::WideToUTF8(kEnrollmentToken1),
            storage.RetrieveEnrollmentToken());
  // The enrollment token should be cached in memory and not read from the
  // system again.
  ASSERT_TRUE(SetEnrollmentToken(kEnrollmentToken2));
  EXPECT_EQ(base::WideToUTF8(kEnrollmentToken1),
            storage.RetrieveEnrollmentToken());
}

TEST_F(BrowserDMTokenStorageWinTest, RetrieveDMToken) {
  ASSERT_TRUE(SetMachineGuid(kClientId1));

  // The DM token will not be read from the system if there is no enrollment
  // token.
  ASSERT_TRUE(SetDMToken(kDMToken1));
  BrowserDMTokenStorageWin storage;
  EXPECT_THAT(storage.RetrieveDMToken(), IsEmpty());

  // With the enrollment token now set, an already initialized
  // BrowserDMTokenStorageWin should still return empty DM token because it
  // should cache the value.
  ASSERT_TRUE(SetEnrollmentToken(kEnrollmentToken1));
  EXPECT_THAT(storage.RetrieveDMToken(), IsEmpty());

  // But if enrollment token is set when BrowserDMTokenStorageWin is first used,
  // it should return the DM token.
  BrowserDMTokenStorageWin storage2;
  EXPECT_EQ(std::string(kDMToken1), storage2.RetrieveDMToken());

  // The DM token should be cached in memory and not read from the system again.
  ASSERT_TRUE(SetDMToken(kDMToken2));
  EXPECT_EQ(std::string(kDMToken1), storage2.RetrieveDMToken());
}
}  // namespace policy
