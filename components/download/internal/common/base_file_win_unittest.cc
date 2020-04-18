// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/common/base_file.h"

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "components/download/public/common/download_interrupt_reasons.h"
#include "components/download/public/common/download_item.h"
#include "net/base/filename_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace download {

TEST(BaseFileWin, AnnotateWithSourceInformation) {
  const base::FilePath::CharType kZoneIdentifierStreamName[] =
      FILE_PATH_LITERAL(":Zone.Identifier");
  const char kInternetZoneIdentifierString[] = "[ZoneTransfer]\r\nZoneId=3\r\n";

  struct {
    const char* const url;
    const char* const referrer;
    bool expected_internet_zone;
  } kTestCases[] = {
      // Test cases where we expect a MOTW.
      {"http://example.com", "http://example.com", true},
      {"", "http://example.com", true},
      {"", "", true},
      {"http://example.com", "", true},
      {"data:text/plain,Foo", "http://example.com", true},
      {"data:text/plain,Foo", "", true},
      {"data:text/plain,Foo", "data:text/plain,Bar", true},
      {"data:text/plain,Foo", "ftp://localhost/foo", true},
      {"http://example.com", "http://localhost/foo", true},
      {"ftp://example.com/foo", "", true},

      // Test cases where we don't expect a MOTW. These test cases result in
      // different behavior across Windows versions.
      {"ftp://localhost/foo", "", false},
      {"http://localhost/foo", "", false},
      {"", "http://localhost/foo", false},
      {"file:///exists.txt", "", false},
      {"file:///exists.txt", "http://example.com", false},
      {"file:///does-not-exist.txt", "", false},
  };

  base::ScopedTempDir target_directory;
  ASSERT_TRUE(target_directory.CreateUniqueTempDir());

  ASSERT_EQ(
      6, base::WriteFile(target_directory.GetPath().AppendASCII("exists.txt"),
                         "Exists", 6));

  for (const auto& test_case : kTestCases) {
    GURL url(test_case.url);
    GURL referrer(test_case.referrer);

    // Resolve file:// URLs relative to our temp directory.
    if (url.SchemeIsFile()) {
      base::FilePath relative_path =
          base::FilePath().AppendASCII(url.path().substr(1));
      url = net::FilePathToFileURL(
          target_directory.GetPath().Append(relative_path));
    }

    SCOPED_TRACE(::testing::Message() << "Source URL: " << url.spec()
                                      << " Referrer: " << test_case.referrer);

    BaseFile base_file(download::DownloadItem::kInvalidId);
    int64_t bytes_wasted = 0;  // unused
    ASSERT_EQ(DOWNLOAD_INTERRUPT_REASON_NONE,
              base_file.Initialize(base::FilePath(), target_directory.GetPath(),
                                   base::File(), 0, std::string(),
                                   std::unique_ptr<crypto::SecureHash>(), false,
                                   &bytes_wasted));
    ASSERT_FALSE(base_file.full_path().empty());
    ASSERT_EQ(DOWNLOAD_INTERRUPT_REASON_NONE,
              base_file.Rename(
                  target_directory.GetPath().AppendASCII("test_file.doc")));
    ASSERT_EQ(DOWNLOAD_INTERRUPT_REASON_NONE,
              base_file.AnnotateWithSourceInformation(
                  "7B2CEE7C-DC81-4160-86F1-9C968597118F", url, referrer));
    base_file.Detach();
    base_file.Finish();

    base::FilePath path = base_file.full_path();
    base::FilePath zone_identifier_stream(path.value() +
                                          kZoneIdentifierStreamName);

    ASSERT_TRUE(base::PathExists(path));

    std::string zone_identifier;
    base::ReadFileToString(zone_identifier_stream, &zone_identifier);

    if (test_case.expected_internet_zone) {
      EXPECT_STREQ(kInternetZoneIdentifierString, zone_identifier.c_str());
    } else {
      // Seeing an unexpected zone identifier is not an error, but we log a
      // warning just the same so that such cases can be identified during
      // manual testing.
      if (zone_identifier == kInternetZoneIdentifierString) {
        LOG(WARNING) << "Unexpected internet zone annotation for Source:"
                     << url.spec() << " Referrer:" << test_case.referrer;
      } else if (!zone_identifier.empty()) {
        LOG(WARNING) << "Unexpected zone annotation for Source:" << url.spec()
                     << " Referrer:" << test_case.referrer
                     << " Annotation:" << std::endl
                     << zone_identifier;
      }
    }
    base::DeleteFile(path, false);
  }
}

}  // namespace download
