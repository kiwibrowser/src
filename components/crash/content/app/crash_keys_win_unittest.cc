// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/content/app/crash_keys_win.h"

#include <stddef.h>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/strings/stringprintf.h"
#include "components/crash/content/app/crash_reporter_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/breakpad/breakpad/src/client/windows/common/ipc_protocol.h"

namespace breakpad {

using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SetArgPointee;

class MockCrashReporterClient : public crash_reporter::CrashReporterClient {
 public:
  MOCK_METHOD1(GetAlternativeCrashDumpLocation,
               bool(base::string16* crash_dir));
  MOCK_METHOD5(GetProductNameAndVersion, void(const base::string16& exe_path,
                                              base::string16* product_name,
                                              base::string16* version,
                                              base::string16* special_build,
                                              base::string16* channel_name));
  MOCK_METHOD3(ShouldShowRestartDialog, bool(base::string16* title,
                                             base::string16* message,
                                             bool* is_rtl_locale));
  MOCK_METHOD0(AboutToRestart, bool());
  MOCK_METHOD1(GetDeferredUploadsSupported, bool(bool is_per_user_install));
  MOCK_METHOD0(GetIsPerUserInstall, bool());
  MOCK_METHOD0(GetShouldDumpLargerDumps, bool());
  MOCK_METHOD0(GetResultCodeRespawnFailed, int());
  MOCK_METHOD0(InitBrowserCrashDumpsRegKey, void());
  MOCK_METHOD1(RecordCrashDumpAttempt, void(bool is_real_crash));

  MOCK_METHOD2(GetProductNameAndVersion, void(std::string* product_name,
                                                std::string* version));
  MOCK_METHOD0(GetReporterLogFilename, base::FilePath());
  MOCK_METHOD1(GetCrashDumpLocation, bool(base::string16* crash_dir));
  MOCK_METHOD0(IsRunningUnattended, bool());
  MOCK_METHOD0(GetCollectStatsConsent, bool());
  MOCK_METHOD1(ReportingIsEnforcedByPolicy, bool(bool* breakpad_enabled));
  MOCK_METHOD0(GetAndroidMinidumpDescriptor, int());
  MOCK_METHOD1(EnableBreakpadForProcess, bool(const std::string& process_type));
};

class CrashKeysWinTest : public testing::Test {
 public:
  size_t CountKeyValueOccurences(
      const google_breakpad::CustomClientInfo* client_info,
      const wchar_t* key, const wchar_t* value);

 protected:
  testing::StrictMock<MockCrashReporterClient> crash_client_;
};

size_t CrashKeysWinTest::CountKeyValueOccurences(
    const google_breakpad::CustomClientInfo* client_info,
    const wchar_t* key, const wchar_t* value) {
  size_t occurrences = 0;
  for (size_t i = 0; i < client_info->count; ++i) {
    if (wcscmp(client_info->entries[i].name, key) == 0 &&
        wcscmp(client_info->entries[i].value, value) == 0) {
      ++occurrences;
    }
  }

  return occurrences;
}

TEST_F(CrashKeysWinTest, RecordsSelf) {
  ASSERT_FALSE(CrashKeysWin::keeper());

  {
    CrashKeysWin crash_keys;

    ASSERT_EQ(&crash_keys, CrashKeysWin::keeper());
  }

  ASSERT_FALSE(CrashKeysWin::keeper());
}

// Tests the crash keys set up for the most common official build consumer
// scenario. No policy controls, not running unattended and no explicit
// switches.
TEST_F(CrashKeysWinTest, OfficialLikeKeys) {
  CrashKeysWin crash_keys;

  const base::string16 kExePath(L"C:\\temp\\exe_path.exe");
  // The exe path ought to get passed through to the breakpad client.
  EXPECT_CALL(crash_client_, GetProductNameAndVersion(kExePath, _, _, _, _))
      .WillRepeatedly(DoAll(
          SetArgPointee<1>(L"SomeProdName"),
          SetArgPointee<2>(L"1.2.3.4"),
          SetArgPointee<3>(L""),
          SetArgPointee<4>(L"-devm")));

  EXPECT_CALL(crash_client_, GetAlternativeCrashDumpLocation(_))
      .WillRepeatedly(DoAll(
          SetArgPointee<0>(L"C:\\temp"),
          Return(false)));

  EXPECT_CALL(crash_client_, ReportingIsEnforcedByPolicy(_))
      .WillRepeatedly(Return(false));

  EXPECT_CALL(crash_client_,  IsRunningUnattended())
      .WillRepeatedly(Return(false));

  // Provide an empty command line.
  base::CommandLine cmd_line(base::CommandLine::NO_PROGRAM);
  google_breakpad::CustomClientInfo* info =
      crash_keys.GetCustomInfo(kExePath,
                               L"made_up_type",
                               L"temporary",
                               &cmd_line,
                               &crash_client_);

  ASSERT_TRUE(info != nullptr);
  ASSERT_TRUE(info->entries != nullptr);

  // We expect 7 fixed keys and a "freeboard" of 256 keys for dynamic entries.
  EXPECT_EQ(256U + 7U, info->count);

  EXPECT_EQ(1u, CountKeyValueOccurences(info, L"ver", L"1.2.3.4"));
  EXPECT_EQ(1u, CountKeyValueOccurences(info, L"prod", L"SomeProdName"));
  EXPECT_EQ(1u, CountKeyValueOccurences(info, L"plat", L"Win32"));
  EXPECT_EQ(1u, CountKeyValueOccurences(info, L"ptype", L"made_up_type"));
  std::wstring pid_str(base::StringPrintf(L"%d", ::GetCurrentProcessId()));
  EXPECT_EQ(1u, CountKeyValueOccurences(info, L"pid", pid_str.c_str()));
  EXPECT_EQ(1u, CountKeyValueOccurences(info, L"channel", L"-devm"));
  EXPECT_EQ(1u, CountKeyValueOccurences(info, L"profile-type", L"temporary"));
  EXPECT_EQ(256u, CountKeyValueOccurences(info, L"unspecified-crash-key", L""));
}

}  // namespace breakpad
