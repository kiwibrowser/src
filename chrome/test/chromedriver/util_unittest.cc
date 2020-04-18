// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/base64.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "chrome/test/chromedriver/chrome/status.h"
#include "chrome/test/chromedriver/util.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(UnzipSoleFile, Entry) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  std::string data;
  // A zip entry sent from a Java WebDriver client (v2.20) that contains a
  // file with the contents "COW\n".
  const char kBase64ZipEntry[] =
      "UEsDBBQACAAIAJpyXEAAAAAAAAAAAAAAAAAEAAAAdGVzdHP2D+"
      "cCAFBLBwi/wAzGBgAAAAQAAAA=";
  ASSERT_TRUE(base::Base64Decode(kBase64ZipEntry, &data));
  base::FilePath file;
  Status status = UnzipSoleFile(temp_dir.GetPath(), data, &file);
  ASSERT_EQ(kOk, status.code()) << status.message();
  std::string contents;
  ASSERT_TRUE(base::ReadFileToString(file, &contents));
  ASSERT_STREQ("COW\n", contents.c_str());
}

TEST(UnzipSoleFile, Archive) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  std::string data;
  // A zip archive sent from a Python WebDriver client that contains a
  // file with the contents "COW\n".
  const char kBase64ZipArchive[] =
      "UEsDBBQAAAAAAMROi0K/wAzGBAAAAAQAAAADAAAAbW9vQ09XClBLAQIUAxQAAAAAAMROi0K/"
      "wAzGBAAAAAQAAAADAAAAAAAAAAAAAACggQAAAABtb29QSwUGAAAAAAEAAQAxAAAAJQAAAAA"
      "A";
  ASSERT_TRUE(base::Base64Decode(kBase64ZipArchive, &data));
  base::FilePath file;
  Status status = UnzipSoleFile(temp_dir.GetPath(), data, &file);
  ASSERT_EQ(kOk, status.code()) << status.message();
  std::string contents;
  ASSERT_TRUE(base::ReadFileToString(file, &contents));
  ASSERT_STREQ("COW\n", contents.c_str());
}
