// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/hwid_checker.h"

#include <cstdio>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/sys_info.h"
#include "chrome/common/chrome_switches.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/system/statistics_provider.h"
#include "content/public/common/content_switches.h"
#include "third_party/re2/src/re2/re2.h"
#include "third_party/zlib/zlib.h"

namespace {

unsigned CalculateCRC32(const std::string& data) {
  return static_cast<unsigned>(
      crc32(0, reinterpret_cast<const Bytef*>(data.c_str()), data.length()));
}

std::string CalculateHWIDv2Checksum(const std::string& data) {
  unsigned crc32 = CalculateCRC32(data);
  // We take four least significant decimal digits of CRC-32.
  char checksum[5];
  int snprintf_result = snprintf(checksum, 5, "%04u", crc32 % 10000);
  LOG_ASSERT(snprintf_result == 4);
  return checksum;
}

bool IsCorrectHWIDv2(const std::string& hwid) {
  std::string body;
  std::string checksum;
  if (!RE2::FullMatch(hwid, "([\\s\\S]*) (\\d{4})", &body, &checksum))
    return false;
  return CalculateHWIDv2Checksum(body) == checksum;
}

bool IsExceptionalHWID(const std::string& hwid) {
  return RE2::PartialMatch(hwid, "^(SPRING [A-D])|(FALCO A)");
}

std::string CalculateExceptionalHWIDChecksum(const std::string& data) {
  static const char base32_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
  unsigned crc32 = CalculateCRC32(data);
  // We take 10 least significant bits of CRC-32 and encode them in 2 characters
  // using Base32 alphabet.
  std::string checksum;
  checksum += base32_alphabet[(crc32 >> 5) & 0x1f];
  checksum += base32_alphabet[crc32 & 0x1f];
  return checksum;
}

bool IsCorrectExceptionalHWID(const std::string& hwid) {
  if (!IsExceptionalHWID(hwid))
    return false;
  std::string bom;
  if (!RE2::FullMatch(hwid, "[A-Z0-9]+ ((?:[A-Z2-7]{4}-)*[A-Z2-7]{1,4})", &bom))
    return false;
  if (bom.length() < 2)
    return false;
  std::string hwid_without_dashes;
  base::RemoveChars(hwid, "-", &hwid_without_dashes);
  LOG_ASSERT(hwid_without_dashes.length() >= 2);
  std::string not_checksum =
      hwid_without_dashes.substr(0, hwid_without_dashes.length() - 2);
  std::string checksum =
      hwid_without_dashes.substr(hwid_without_dashes.length() - 2);
  return CalculateExceptionalHWIDChecksum(not_checksum) == checksum;
}

std::string CalculateHWIDv3Checksum(const std::string& data) {
  static const char base8_alphabet[] = "23456789";
  static const char base32_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
  unsigned crc32 = CalculateCRC32(data);
  // We take 8 least significant bits of CRC-32 and encode them in 2 characters.
  std::string checksum;
  checksum += base8_alphabet[(crc32 >> 5) & 0x7];
  checksum += base32_alphabet[crc32 & 0x1f];
  return checksum;
}

bool IsCorrectHWIDv3(const std::string& hwid) {
  if (IsExceptionalHWID(hwid))
    return false;
  std::string regex =
      "([A-Z0-9]+ (?:[A-Z2-7][2-9][A-Z2-7]-)*[A-Z2-7])([2-9][A-Z2-7])";
  std::string not_checksum, checksum;
  if (!RE2::FullMatch(hwid, regex, &not_checksum, &checksum))
    return false;
  base::RemoveChars(not_checksum, "-", &not_checksum);
  return CalculateHWIDv3Checksum(not_checksum) == checksum;
}

}  // anonymous namespace

namespace chromeos {

bool IsHWIDCorrect(const std::string& hwid) {
  return IsCorrectHWIDv2(hwid) || IsCorrectExceptionalHWID(hwid) ||
         IsCorrectHWIDv3(hwid);
}

bool IsMachineHWIDCorrect() {
#if !defined(GOOGLE_CHROME_BUILD)
  return true;
#endif
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(::switches::kTestType))
    return true;
  if (!base::SysInfo::IsRunningOnChromeOS())
    return true;

  chromeos::system::StatisticsProvider* stats =
      chromeos::system::StatisticsProvider::GetInstance();
  if (stats->IsRunningOnVm())
    return true;

  std::string hwid;
  if (!stats->GetMachineStatistic(chromeos::system::kHardwareClassKey, &hwid)) {
    LOG(ERROR) << "Couldn't get machine statistic 'hardware_class'.";
    return false;
  }
  if (!chromeos::IsHWIDCorrect(hwid)) {
    LOG(ERROR) << "Machine has malformed HWID '" << hwid << "'. ";
    return false;
  }
  return true;
}

}  // namespace chromeos
