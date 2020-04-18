// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/importer/firefox_importer_utils.h"

#include <stddef.h>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/values.h"
#include "chrome/grit/generated_resources.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

struct GetPrefsJsValueCase {
  std::string prefs_content;
  std::string pref_name;
  std::string pref_value;
} GetPrefsJsValueCases[] = {
  // Basic case. Single pref, unquoted value.
  { "user_pref(\"foo.bar\", 1);", "foo.bar", "1" },
  // Value is quoted. Quotes should be stripped.
  { "user_pref(\"foo.bar\", \"1\");", "foo.bar", "1" },
  // Value has parens.
  { "user_pref(\"foo.bar\", \"Value (detail)\");",
    "foo.bar", "Value (detail)" },
  // Multi-line case.
  { "user_pref(\"foo.bar\", 1);\n"
    "user_pref(\"foo.baz\", 2);\n"
    "user_pref(\"foo.bag\", 3);",
    "foo.baz", "2" },
  // Malformed content.
  { "user_pref(\"foo.bar\", 1);\n"
    "user_pref(\"foo.baz\", 2;\n"
    "user_pref(\"foo.bag\", 3);",
    "foo.baz", "" },
  // Malformed content.
  { "uesr_pref(\"foo.bar\", 1);", "foo.bar", "" },
};

struct GetFirefoxImporterNameCase {
  std::string app_ini_content;
  int resource_id;
} GetFirefoxImporterNameCases[] = {
  // Basic case
  { "[App]\n"
    "Vendor=Mozilla\n"
    "Name=iceweasel\n"
    "Version=10.0.6\n"
    "BuildID=20120717115048\n"
    "ID={ec8030f7-c20a-464f-9b0e-13a3a9e97384}",
    IDS_IMPORT_FROM_ICEWEASEL },
  // Whitespace
  { " \t[App] \n"
    "Vendor=Mozilla\n"
    "   Name=Firefox\t \r\n"
    "Version=10.0.6\n",
    IDS_IMPORT_FROM_FIREFOX },
  // No Name setting
  { "[App]\n"
    "Vendor=Mozilla\n"
    "Version=10.0.6\n"
    "BuildID=20120717115048\n"
    "ID={ec8030f7-c20a-464f-9b0e-13a3a9e97384}",
    IDS_IMPORT_FROM_FIREFOX },
  // No [App] section
  { "[Foo]\n"
    "Vendor=Mozilla\n"
    "Name=Foo\n",
    IDS_IMPORT_FROM_FIREFOX },
  // Multiple Name settings in different sections
  { "[Foo]\n"
    "Vendor=Mozilla\n"
    "Name=Firefox\n"
    "[App]\n"
    "Profile=mozilla/firefox\n"
    "Name=iceweasel\n"
    "[Bar]\n"
    "Name=Bar\n"
    "ID={ec8030f7-c20a-464f-9b0e-13a3a9e97384}",
    IDS_IMPORT_FROM_ICEWEASEL },
  // Case-insensitivity
  { "[App]\n"
    "Vendor=Mozilla\n"
    "Name=IceWeasel\n"
    "Version=10.0.6\n",
    IDS_IMPORT_FROM_ICEWEASEL },
  // Empty file
  { "", IDS_IMPORT_FROM_FIREFOX }
};

}  // anonymous namespace

TEST(FirefoxImporterUtilsTest, GetPrefsJsValue) {
  for (size_t i = 0; i < arraysize(GetPrefsJsValueCases); ++i) {
    EXPECT_EQ(
      GetPrefsJsValueCases[i].pref_value,
      GetPrefsJsValue(GetPrefsJsValueCases[i].prefs_content,
                      GetPrefsJsValueCases[i].pref_name));
  }
}

TEST(FirefoxImporterUtilsTest, GetFirefoxImporterName) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  const base::FilePath app_ini_file(
      temp_dir.GetPath().AppendASCII("application.ini"));
  for (size_t i = 0; i < arraysize(GetFirefoxImporterNameCases); ++i) {
    base::WriteFile(app_ini_file,
                    GetFirefoxImporterNameCases[i].app_ini_content.c_str(),
                    GetFirefoxImporterNameCases[i].app_ini_content.size());
    EXPECT_EQ(
        GetFirefoxImporterName(temp_dir.GetPath()),
        l10n_util::GetStringUTF16(GetFirefoxImporterNameCases[i].resource_id));
  }
  EXPECT_EQ(l10n_util::GetStringUTF16(
          IDS_IMPORT_FROM_FIREFOX),
      GetFirefoxImporterName(base::FilePath(
                                        FILE_PATH_LITERAL("/invalid/path"))));
}

TEST(FirefoxImporterUtilsTest, GetFirefoxProfilePath) {
  base::DictionaryValue no_profiles;
  EXPECT_EQ("",
            GetFirefoxProfilePathFromDictionary(no_profiles).MaybeAsASCII());

  base::DictionaryValue single_profile;
  single_profile.SetString("Profile0.Path", "first");
  single_profile.SetString("Profile0.IsRelative", "0");
  single_profile.SetString("Profile0.Default", "1");
  EXPECT_EQ("first",
            GetFirefoxProfilePathFromDictionary(single_profile).MaybeAsASCII());

  base::DictionaryValue no_default;
  no_default.SetString("Profile0.Path", "first");
  no_default.SetString("Profile0.IsRelative", "0");
  no_default.SetString("Profile1.Path", "second");
  no_default.SetString("Profile1.IsRelative", "0");
  EXPECT_EQ("first",
            GetFirefoxProfilePathFromDictionary(no_default).MaybeAsASCII());

  base::DictionaryValue default_first;
  default_first.SetString("Profile0.Path", "first");
  default_first.SetString("Profile0.IsRelative", "0");
  default_first.SetString("Profile0.Default", "1");
  default_first.SetString("Profile1.Path", "second");
  default_first.SetString("Profile1.IsRelative", "0");
  EXPECT_EQ("first",
            GetFirefoxProfilePathFromDictionary(default_first).MaybeAsASCII());

  base::DictionaryValue default_second;
  default_second.SetString("Profile0.Path", "first");
  default_second.SetString("Profile0.IsRelative", "0");
  default_second.SetString("Profile1.Path", "second");
  default_second.SetString("Profile1.IsRelative", "0");
  default_second.SetString("Profile1.Default", "1");
  EXPECT_EQ("second",
            GetFirefoxProfilePathFromDictionary(default_second).MaybeAsASCII());
}
