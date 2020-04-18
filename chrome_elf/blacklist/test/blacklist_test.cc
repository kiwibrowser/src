// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <memory>

#include "base/environment.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/i18n/case_conversion.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/scoped_native_library.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_reg_util_win.h"
#include "base/win/registry.h"
#include "chrome/common/chrome_version.h"
#include "chrome/install_static/install_util.h"
#include "chrome_elf/blacklist/blacklist.h"
#include "chrome_elf/chrome_elf_constants.h"
#include "chrome_elf/nt_registry/nt_registry.h"
#include "testing/gtest/include/gtest/gtest.h"

const wchar_t kTestDllName1[] = L"blacklist_test_dll_1.dll";
const wchar_t kTestDllName2[] = L"blacklist_test_dll_2.dll";
const wchar_t kTestDllName3[] = L"blacklist_test_dll_3.dll";

const wchar_t kDll2Beacon[] = L"{F70A0100-2889-4629-9B44-610FE5C73231}";
const wchar_t kDll3Beacon[] = L"{9E056AEC-169E-400c-B2D0-5A07E3ACE2EB}";

extern const wchar_t* kEnvVars[];

namespace {

// Functions we need from blacklist_test_main_dll.dll
typedef bool (*TestDll_AddDllToBlacklistFunction)(const wchar_t* dll_name);
typedef int (*TestDll_BlacklistSizeFunction)();
typedef void (*TestDll_BlockedDllFunction)(size_t blocked_index);
typedef bool (*TestDll_IsBlacklistInitializedFunction)();
typedef bool (*TestDll_RemoveDllFromBlacklistFunction)(const wchar_t* dll_name);
typedef bool (*TestDll_SuccessfullyBlockedFunction)(
    const wchar_t** blocked_dlls,
    int* size);
typedef void (*InitTestDllFunction)();

TestDll_AddDllToBlacklistFunction TestDll_AddDllToBlacklist = nullptr;
TestDll_BlacklistSizeFunction TestDll_BlacklistSize = nullptr;
TestDll_BlockedDllFunction TestDll_BlockedDll = nullptr;
TestDll_IsBlacklistInitializedFunction TestDll_IsBlacklistInitialized = nullptr;
TestDll_RemoveDllFromBlacklistFunction TestDll_RemoveDllFromBlacklist = nullptr;
TestDll_SuccessfullyBlockedFunction TestDll_SuccessfullyBlocked = nullptr;
InitTestDllFunction InitTestDll = nullptr;

struct TestData {
  const wchar_t* dll_name;
  const wchar_t* dll_beacon;
} test_data[] = {
    {kTestDllName2, kDll2Beacon},
    {kTestDllName3, kDll3Beacon}
};

class BlacklistTest : public testing::Test {
 protected:
  BlacklistTest() : override_manager_(), num_initially_blocked_(0) {
  }

  void CheckBlacklistedDllsNotLoaded() {
    base::FilePath current_dir;
    ASSERT_TRUE(base::PathService::Get(base::DIR_EXE, &current_dir));

    for (size_t i = 0; i < arraysize(test_data); ++i) {
      // Ensure that the dll has not been loaded both by inspecting the handle
      // returned by LoadLibrary and by looking for an environment variable that
      // is set when the DLL's entry point is called.
      base::ScopedNativeLibrary dll_blacklisted(
          current_dir.Append(test_data[i].dll_name));
      EXPECT_FALSE(dll_blacklisted.is_valid());
      EXPECT_EQ(0u, ::GetEnvironmentVariable(test_data[i].dll_beacon, NULL, 0));
      dll_blacklisted.Reset(NULL);

      // Ensure that the dll is recorded as blocked.
      int array_size = 1 + num_initially_blocked_;
      std::vector<const wchar_t*> blocked_dlls(array_size);
      TestDll_SuccessfullyBlocked(&blocked_dlls[0], &array_size);
      EXPECT_EQ(1 + num_initially_blocked_, array_size);
      EXPECT_STREQ(test_data[i].dll_name, blocked_dlls[num_initially_blocked_]);

      // Remove the DLL from the blacklist. Ensure that it loads and that its
      // entry point was called.
      EXPECT_TRUE(TestDll_RemoveDllFromBlacklist(test_data[i].dll_name));
      base::ScopedNativeLibrary dll(current_dir.Append(test_data[i].dll_name));
      EXPECT_TRUE(dll.is_valid());
      EXPECT_NE(0u, ::GetEnvironmentVariable(test_data[i].dll_beacon, NULL, 0));
      dll.Reset(NULL);

      ::SetEnvironmentVariable(test_data[i].dll_beacon, NULL);

      // Ensure that the dll won't load even if the name has different
      // capitalization.
      base::string16 uppercase_name =
          base::i18n::ToUpper(test_data[i].dll_name);
      EXPECT_TRUE(TestDll_AddDllToBlacklist(uppercase_name.c_str()));
      base::ScopedNativeLibrary dll_blacklisted_different_case(
          current_dir.Append(test_data[i].dll_name));
      EXPECT_FALSE(dll_blacklisted_different_case.is_valid());
      EXPECT_EQ(0u, ::GetEnvironmentVariable(test_data[i].dll_beacon, NULL, 0));
      dll_blacklisted_different_case.Reset(NULL);

      EXPECT_TRUE(TestDll_RemoveDllFromBlacklist(uppercase_name.c_str()));

      // The blocked dll was removed, so the number of blocked dlls should
      // return to what it originally was.
      int num_blocked_dlls = 0;
      TestDll_SuccessfullyBlocked(NULL, &num_blocked_dlls);
      EXPECT_EQ(num_initially_blocked_, num_blocked_dlls);
    }
  }

  std::unique_ptr<base::win::RegKey> blacklist_registry_key_;
  registry_util::RegistryOverrideManager override_manager_;

  // The number of dlls initially blocked by the blacklist.
  int num_initially_blocked_;

 private:
  // This function puts registry-key redirection paths into
  // process-specific environment variables, for our test DLLs to access.
  // This will only work as long as the IPC is within the same process.
  void IpcOverrides() {
    base::string16 temp = nt::GetTestingOverride(nt::HKCU);
    if (!temp.empty())
      ASSERT_TRUE(::SetEnvironmentVariableW(L"hkcu_override", temp.c_str()));
    temp = nt::GetTestingOverride(nt::HKLM);
    if (!temp.empty())
      ASSERT_TRUE(::SetEnvironmentVariableW(L"hklm_override", temp.c_str()));
  }

  void SetUp() override {
    base::string16 temp;
    ASSERT_NO_FATAL_FAILURE(
        override_manager_.OverrideRegistry(HKEY_CURRENT_USER, &temp));
    ASSERT_TRUE(nt::SetTestingOverride(nt::HKCU, temp));

    // Make the override path available to our test DLL.
    IpcOverrides();

    // Load the main test Dll now.
    // Note: this has to happen after we set up the registry overrides.
    HMODULE dll = nullptr;
    dll = ::LoadLibraryW(L"blacklist_test_main_dll.dll");
    if (!dll)
      return;
    TestDll_AddDllToBlacklist =
        reinterpret_cast<TestDll_AddDllToBlacklistFunction>(
            ::GetProcAddress(dll, "TestDll_AddDllToBlacklist"));
    TestDll_BlacklistSize = reinterpret_cast<TestDll_BlacklistSizeFunction>(
        ::GetProcAddress(dll, "TestDll_BlacklistSize"));
    TestDll_BlockedDll = reinterpret_cast<TestDll_BlockedDllFunction>(
        ::GetProcAddress(dll, "TestDll_BlockedDll"));
    TestDll_IsBlacklistInitialized =
        reinterpret_cast<TestDll_IsBlacklistInitializedFunction>(
            ::GetProcAddress(dll, "TestDll_IsBlacklistInitialized"));
    TestDll_RemoveDllFromBlacklist =
        reinterpret_cast<TestDll_RemoveDllFromBlacklistFunction>(
            ::GetProcAddress(dll, "TestDll_RemoveDllFromBlacklist"));
    TestDll_SuccessfullyBlocked =
        reinterpret_cast<TestDll_SuccessfullyBlockedFunction>(
            ::GetProcAddress(dll, "TestDll_SuccessfullyBlocked"));
    InitTestDll = reinterpret_cast<InitTestDllFunction>(
        ::GetProcAddress(dll, "InitTestDll"));
    if (!TestDll_AddDllToBlacklist || !TestDll_BlacklistSize ||
        !TestDll_BlockedDll || !TestDll_IsBlacklistInitialized ||
        !TestDll_RemoveDllFromBlacklist || !TestDll_SuccessfullyBlocked ||
        !InitTestDll)
      return;

    // We have to call this exported function every time this test setup runs.
    // If the tests are running in single process mode, the test DLL does not
    // get reloaded everytime - but we need to make sure it updates
    // appropriately.
    InitTestDll();

    blacklist_registry_key_.reset(
        new base::win::RegKey(HKEY_CURRENT_USER,
                              install_static::GetRegistryPath()
                                  .append(blacklist::kRegistryBeaconKeyName)
                                  .c_str(),
                              KEY_QUERY_VALUE | KEY_SET_VALUE));

    // Find out how many dlls were blocked before the test starts.
    TestDll_SuccessfullyBlocked(NULL, &num_initially_blocked_);
  }

  void TearDown() override {
    TestDll_RemoveDllFromBlacklist(kTestDllName1);
    TestDll_RemoveDllFromBlacklist(kTestDllName2);
    TestDll_RemoveDllFromBlacklist(kTestDllName3);

    ASSERT_TRUE(nt::SetTestingOverride(nt::HKCU, base::string16()));
  }
};

TEST_F(BlacklistTest, Beacon) {
  // Ensure that the beacon state starts off 'running' for this version.
  LONG result = blacklist_registry_key_->WriteValue(
      blacklist::kBeaconState, blacklist::BLACKLIST_SETUP_RUNNING);
  EXPECT_EQ(ERROR_SUCCESS, result);

  result = blacklist_registry_key_->WriteValue(blacklist::kBeaconVersion,
                                               TEXT(CHROME_VERSION_STRING));
  EXPECT_EQ(ERROR_SUCCESS, result);

  // First call should find the beacon and reset it.
  EXPECT_TRUE(blacklist::ResetBeacon());

  // First call should succeed as the beacon is enabled.
  EXPECT_TRUE(blacklist::LeaveSetupBeacon());
}

TEST_F(BlacklistTest, AddAndRemoveModules) {
  EXPECT_TRUE(TestDll_AddDllToBlacklist(L"foo.dll"));
  // Adding the same item twice should be idempotent.
  EXPECT_TRUE(TestDll_AddDllToBlacklist(L"foo.dll"));
  EXPECT_TRUE(TestDll_RemoveDllFromBlacklist(L"foo.dll"));
  EXPECT_FALSE(TestDll_RemoveDllFromBlacklist(L"foo.dll"));

  // Increase the blacklist size by 1 to include the NULL pointer
  // that marks the end.
  int empty_spaces =
      blacklist::kTroublesomeDllsMaxCount - (TestDll_BlacklistSize() + 1);
  std::vector<base::string16> added_dlls;
  added_dlls.reserve(empty_spaces);
  for (int i = 0; i < empty_spaces; ++i) {
    added_dlls.push_back(base::IntToString16(i) + L".dll");
    EXPECT_TRUE(TestDll_AddDllToBlacklist(added_dlls[i].c_str())) << i;
  }
  EXPECT_FALSE(TestDll_AddDllToBlacklist(L"overflow.dll"));
  for (int i = 0; i < empty_spaces; ++i) {
    EXPECT_TRUE(TestDll_RemoveDllFromBlacklist(added_dlls[i].c_str())) << i;
  }
  EXPECT_FALSE(TestDll_RemoveDllFromBlacklist(added_dlls[0].c_str()));
  EXPECT_FALSE(
      TestDll_RemoveDllFromBlacklist(added_dlls[empty_spaces - 1].c_str()));
}

TEST_F(BlacklistTest, SuccessfullyBlocked) {
  const int initial_size = TestDll_BlacklistSize();

  // Add 5 news dlls to blacklist.
  const int kDesiredBlacklistSize = 5;
  std::vector<base::string16> dlls_to_block;
  for (int i = 0; i < kDesiredBlacklistSize; ++i) {
    dlls_to_block.push_back(base::IntToString16(i) + L".dll");
    ASSERT_TRUE(TestDll_AddDllToBlacklist(dlls_to_block[i].c_str()));
  }

  // Block the dlls, one at a time, and ensure SuccesfullyBlocked correctly
  // passes the list of blocked dlls.
  for (int i = 0; i < kDesiredBlacklistSize; ++i) {
    TestDll_BlockedDll(initial_size + i);

    int size = 0;
    TestDll_SuccessfullyBlocked(NULL, &size);
    ASSERT_EQ(num_initially_blocked_ + i + 1, size);

    std::vector<const wchar_t*> blocked_dlls(size);
    TestDll_SuccessfullyBlocked(&(blocked_dlls[0]), &size);
    ASSERT_EQ(num_initially_blocked_ + i + 1, size);

    for (int j = 0; j <= i; ++j) {
      EXPECT_STREQ(blocked_dlls[num_initially_blocked_ + j],
                   dlls_to_block[j].c_str());
    }
  }

  // Remove the dlls from the blacklist now that we are done.
  for (const auto& dll : dlls_to_block) {
    EXPECT_TRUE(TestDll_RemoveDllFromBlacklist(dll.c_str()));
  }
}

// Disabled due to flakiness.  https://crbug.com/711651.
TEST_F(BlacklistTest, DISABLED_LoadBlacklistedLibrary) {
  base::FilePath current_dir;
  ASSERT_TRUE(base::PathService::Get(base::DIR_EXE, &current_dir));

  // Ensure that the blacklist is loaded.
  ASSERT_TRUE(TestDll_IsBlacklistInitialized());

  // Test that an un-blacklisted DLL can load correctly.
  base::ScopedNativeLibrary dll1(current_dir.Append(kTestDllName1));
  EXPECT_TRUE(dll1.is_valid());
  dll1.Reset(NULL);

  int num_blocked_dlls = 0;
  TestDll_SuccessfullyBlocked(NULL, &num_blocked_dlls);
  EXPECT_EQ(num_initially_blocked_, num_blocked_dlls);

  // Add all DLLs to the blacklist then check they are blocked.
  for (size_t i = 0; i < arraysize(test_data); ++i) {
    EXPECT_TRUE(TestDll_AddDllToBlacklist(test_data[i].dll_name));
  }
  CheckBlacklistedDllsNotLoaded();
}

void TestResetBeacon(std::unique_ptr<base::win::RegKey>& key,
                     DWORD input_state,
                     DWORD expected_output_state) {
  LONG result = key->WriteValue(blacklist::kBeaconState, input_state);
  EXPECT_EQ(ERROR_SUCCESS, result);

  EXPECT_TRUE(blacklist::ResetBeacon());
  DWORD blacklist_state = blacklist::BLACKLIST_STATE_MAX;
  result = key->ReadValueDW(blacklist::kBeaconState, &blacklist_state);
  EXPECT_EQ(ERROR_SUCCESS, result);
  EXPECT_EQ(expected_output_state, blacklist_state);
}

TEST_F(BlacklistTest, ResetBeacon) {
  // Ensure that ResetBeacon resets properly on successful runs and not on
  // failed or disabled runs.
  TestResetBeacon(blacklist_registry_key_,
                  blacklist::BLACKLIST_SETUP_RUNNING,
                  blacklist::BLACKLIST_ENABLED);

  TestResetBeacon(blacklist_registry_key_,
                  blacklist::BLACKLIST_SETUP_FAILED,
                  blacklist::BLACKLIST_SETUP_FAILED);

  TestResetBeacon(blacklist_registry_key_,
                  blacklist::BLACKLIST_DISABLED,
                  blacklist::BLACKLIST_DISABLED);
}

TEST_F(BlacklistTest, SetupFailed) {
  // Ensure that when the number of failed tries reaches the maximum allowed,
  // the blacklist state is set to failed.
  LONG result = blacklist_registry_key_->WriteValue(
      blacklist::kBeaconState, blacklist::BLACKLIST_SETUP_RUNNING);
  EXPECT_EQ(ERROR_SUCCESS, result);

  // Set the attempt count so that on the next failure the blacklist is
  // disabled.
  result = blacklist_registry_key_->WriteValue(
      blacklist::kBeaconAttemptCount, blacklist::kBeaconMaxAttempts - 1);
  EXPECT_EQ(ERROR_SUCCESS, result);

  EXPECT_FALSE(blacklist::LeaveSetupBeacon());

  DWORD attempt_count = 0;
  blacklist_registry_key_->ReadValueDW(blacklist::kBeaconAttemptCount,
                                       &attempt_count);
  EXPECT_EQ(attempt_count, blacklist::kBeaconMaxAttempts);

  DWORD blacklist_state = blacklist::BLACKLIST_STATE_MAX;
  result = blacklist_registry_key_->ReadValueDW(blacklist::kBeaconState,
                                                &blacklist_state);
  EXPECT_EQ(ERROR_SUCCESS, result);
  EXPECT_EQ(blacklist_state,
            static_cast<DWORD>(blacklist::BLACKLIST_SETUP_FAILED));
}

TEST_F(BlacklistTest, SetupSucceeded) {
  // Starting with the enabled beacon should result in the setup running state
  // and the attempt counter reset to zero.
  LONG result = blacklist_registry_key_->WriteValue(
      blacklist::kBeaconState, blacklist::BLACKLIST_ENABLED);
  EXPECT_EQ(ERROR_SUCCESS, result);
  result = blacklist_registry_key_->WriteValue(blacklist::kBeaconAttemptCount,
                                               blacklist::kBeaconMaxAttempts);
  EXPECT_EQ(ERROR_SUCCESS, result);

  EXPECT_TRUE(blacklist::LeaveSetupBeacon());

  DWORD blacklist_state = blacklist::BLACKLIST_STATE_MAX;
  blacklist_registry_key_->ReadValueDW(blacklist::kBeaconState,
                                       &blacklist_state);
  EXPECT_EQ(blacklist_state,
            static_cast<DWORD>(blacklist::BLACKLIST_SETUP_RUNNING));

  DWORD attempt_count = blacklist::kBeaconMaxAttempts;
  blacklist_registry_key_->ReadValueDW(blacklist::kBeaconAttemptCount,
                                       &attempt_count);
  EXPECT_EQ(static_cast<DWORD>(0), attempt_count);
}

}  // namespace
