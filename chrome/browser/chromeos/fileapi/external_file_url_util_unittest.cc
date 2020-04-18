// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/external_file_url_util.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/drive/file_system_core_util.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace {

// Sets up ProfileManager for testing and marks the current thread as UI by
// TestBrowserThreadBundle. We need the thread since Profile objects must be
// touched from UI and hence has CHECK/DCHECKs for it.
class ExternalFileURLUtilTest : public testing::Test {
 protected:
  ExternalFileURLUtilTest()
      : testing_profile_manager_(TestingBrowserProcess::GetGlobal()) {}

  void SetUp() override { ASSERT_TRUE(testing_profile_manager_.SetUp()); }

  TestingProfileManager& testing_profile_manager() {
    return testing_profile_manager_;
  }

  storage::FileSystemURL CreateExpectedURL(const base::FilePath& path) {
    return storage::FileSystemURL::CreateForTest(
        GURL("chrome-extension://xxx"),
        storage::kFileSystemTypeExternal,
        base::FilePath("drive-test-user-hash").Append(path),
        "",
        storage::kFileSystemTypeDrive,
        base::FilePath(),
        "",
        storage::FileSystemMountOption());
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfileManager testing_profile_manager_;
};

}  // namespace

TEST_F(ExternalFileURLUtilTest, FilePathToExternalFileURL) {
  storage::FileSystemURL url;

  // Path with alphabets and numbers.
  url = CreateExpectedURL(base::FilePath("foo/bar012.txt"));
  EXPECT_EQ(url.virtual_path(),
            ExternalFileURLToVirtualPath(FileSystemURLToExternalFileURL(url)));

  // Path with symbols.
  url = CreateExpectedURL(base::FilePath(" !\"#$%&'()*+,-.:;<=>?@[\\]^_`{|}~"));
  EXPECT_EQ(url.virtual_path(),
            ExternalFileURLToVirtualPath(FileSystemURLToExternalFileURL(url)));

  // Path with '%'.
  url = CreateExpectedURL(base::FilePath("%19%20%21.txt"));
  EXPECT_EQ(url.virtual_path(),
            ExternalFileURLToVirtualPath(FileSystemURLToExternalFileURL(url)));

  // Path with multi byte characters.
  base::string16 utf16_string;
  utf16_string.push_back(0x307b);  // HIRAGANA_LETTER_HO
  utf16_string.push_back(0x3052);  // HIRAGANA_LETTER_GE
  url = CreateExpectedURL(
      base::FilePath::FromUTF8Unsafe(base::UTF16ToUTF8(utf16_string) + ".txt"));
  EXPECT_EQ(url.virtual_path().AsUTF8Unsafe(),
            ExternalFileURLToVirtualPath(FileSystemURLToExternalFileURL(url))
                .AsUTF8Unsafe());
}

TEST_F(ExternalFileURLUtilTest, VirtualPathToExternalFileURL) {
  base::FilePath virtual_path(FILE_PATH_LITERAL("foo/bar012.txt"));
  GURL result = VirtualPathToExternalFileURL(virtual_path);
  EXPECT_TRUE(result.is_valid());
  EXPECT_EQ(content::kExternalFileScheme, result.scheme());
  EXPECT_EQ(virtual_path.value(), ExternalFileURLToVirtualPath(result).value());
}

}  // namespace chromeos
