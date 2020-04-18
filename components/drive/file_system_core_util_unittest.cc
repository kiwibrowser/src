// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/file_system_core_util.h"

#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace util {

class FileSystemUtilTest : public testing::Test {
  content::TestBrowserThreadBundle thread_bundle_;
};

TEST_F(FileSystemUtilTest, EscapeUnescapeCacheFileName) {
  const std::string kUnescapedFileName(
      "tmp:`~!@#$%^&*()-_=+[{|]}\\\\;\',<.>/?");
  const std::string kEscapedFileName(
      "tmp:`~!@#$%25^&*()-_=+[{|]}\\\\;\',<%2E>%2F?");
  EXPECT_EQ(kEscapedFileName, EscapeCacheFileName(kUnescapedFileName));
  EXPECT_EQ(kUnescapedFileName, UnescapeCacheFileName(kEscapedFileName));
}

TEST_F(FileSystemUtilTest, NormalizeFileName) {
  EXPECT_EQ("", NormalizeFileName(""));
  EXPECT_EQ("foo", NormalizeFileName("foo"));
  // Slash
  EXPECT_EQ("foo_zzz", NormalizeFileName("foo/zzz"));
  EXPECT_EQ("___", NormalizeFileName("///"));
  // Japanese hiragana "hi" + semi-voiced-mark is normalized to "pi".
  EXPECT_EQ("\xE3\x81\xB4", NormalizeFileName("\xE3\x81\xB2\xE3\x82\x9A"));
  // Dot
  EXPECT_EQ("_", NormalizeFileName("."));
  EXPECT_EQ("_", NormalizeFileName(".."));
  EXPECT_EQ("_", NormalizeFileName("..."));
  EXPECT_EQ(".bashrc", NormalizeFileName(".bashrc"));
  EXPECT_EQ("._", NormalizeFileName("./"));
}

TEST_F(FileSystemUtilTest, GDocFile) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  GURL url(
      "https://docs.google.com/document/d/"
      "1YsCnrMxxgp7LDdtlFDt-WdtEIth89vA9inrILtvK-Ug/edit");
  std::string resource_id("1YsCnrMxxgp7LDdtlFDt-WdtEIth89vA9inrILtvK-Ug");

  // Read and write gdoc.
  base::FilePath file = temp_dir.GetPath().AppendASCII("test.gdoc");
  EXPECT_TRUE(CreateGDocFile(file, url, resource_id));
  EXPECT_EQ(url, ReadUrlFromGDocFile(file));
  EXPECT_EQ(resource_id, ReadResourceIdFromGDocFile(file));

  // Read and write gsheet.
  file = temp_dir.GetPath().AppendASCII("test.gsheet");
  EXPECT_TRUE(CreateGDocFile(file, url, resource_id));
  EXPECT_EQ(url, ReadUrlFromGDocFile(file));
  EXPECT_EQ(resource_id, ReadResourceIdFromGDocFile(file));

  // Read and write gslides.
  file = temp_dir.GetPath().AppendASCII("test.gslides");
  EXPECT_TRUE(CreateGDocFile(file, url, resource_id));
  EXPECT_EQ(url, ReadUrlFromGDocFile(file));
  EXPECT_EQ(resource_id, ReadResourceIdFromGDocFile(file));

  // Read and write gdraw.
  file = temp_dir.GetPath().AppendASCII("test.gdraw");
  EXPECT_TRUE(CreateGDocFile(file, url, resource_id));
  EXPECT_EQ(url, ReadUrlFromGDocFile(file));
  EXPECT_EQ(resource_id, ReadResourceIdFromGDocFile(file));

  // Read and write gtable.
  file = temp_dir.GetPath().AppendASCII("test.gtable");
  EXPECT_TRUE(CreateGDocFile(file, url, resource_id));
  EXPECT_EQ(url, ReadUrlFromGDocFile(file));
  EXPECT_EQ(resource_id, ReadResourceIdFromGDocFile(file));

  // Non GDoc file.
  file = temp_dir.GetPath().AppendASCII("test.txt");
  std::string data = "Hello world!";
  EXPECT_TRUE(google_apis::test_util::WriteStringToFile(file, data));
  EXPECT_TRUE(ReadUrlFromGDocFile(file).is_empty());
  EXPECT_TRUE(ReadResourceIdFromGDocFile(file).empty());
}

}  // namespace util
}  // namespace drive
