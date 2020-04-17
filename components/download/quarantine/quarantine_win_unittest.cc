// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include <wininet.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/test_file_util.h"
#include "components/download/quarantine/quarantine.h"
#include "net/base/filename_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace download {

namespace {

const char kDummySourceUrl[] = "https://example.com/foo";
const char kDummyReferrerUrl[] = "https://example.com/referrer";
const char kDummyClientGuid[] = "A1B69307-8FA2-4B6F-9181-EA06051A48A7";

const char kMotwForInternetZone[] = "[ZoneTransfer]\r\nZoneId=3\r\n";
const base::FilePath::CharType kMotwStreamSuffix[] =
    FILE_PATH_LITERAL(":Zone.Identifier");

const char kTestData[] = "Hello world!";

const char* const kUntrustedURLs[] = {
    "http://example.com/foo",
    "https://example.com/foo",
    "ftp://example.com/foo",
    "ftp://example.com:2121/foo",
    "data:text/plain,Hello%20world",
    "blob://example.com/126278b3-58f3-4b4a-a914-1d1185d634f6",
    "about:internet",
    ""};

}  // namespace

// If the file is missing, the QuarantineFile() call should return FILE_MISSING.
TEST(QuarantineWinTest, MissingFile) {
  base::ScopedTempDir test_dir;
  ASSERT_TRUE(test_dir.CreateUniqueTempDir());

  EXPECT_EQ(QuarantineFileResult::FILE_MISSING,
            QuarantineFile(test_dir.GetPath().AppendASCII("does-not-exist.exe"),
                           GURL(kDummySourceUrl), GURL(kDummyReferrerUrl),
                           kDummyClientGuid));
}

// On Windows systems, files downloaded from a local source are considered
// trustworthy. Hence they aren't annotated with source information. This test
// verifies this behavior since the other tests in this suite would pass with a
// false positive if local files are being annotated with the MOTW for the
// internet zone.
TEST(QuarantineWinTest, LocalFile_DependsOnLocalConfig) {
  base::HistogramTester histogram_tester;
  base::ScopedTempDir test_dir;
  ASSERT_TRUE(test_dir.CreateUniqueTempDir());
  base::FilePath test_file = test_dir.GetPath().AppendASCII("foo.exe");

  const char* const kLocalSourceURLs[] = {"http://localhost/foo",
                                          "file:///C:/some-local-dir/foo.exe"};

  for (const char* source_url : kLocalSourceURLs) {
    SCOPED_TRACE(::testing::Message() << "Trying URL " << source_url);
    ASSERT_EQ(static_cast<int>(arraysize(kTestData)),
              base::WriteFile(test_file, kTestData, arraysize(kTestData)));

    EXPECT_EQ(
        QuarantineFileResult::OK,
        QuarantineFile(test_file, GURL(source_url), GURL(), kDummyClientGuid));

    std::string motw_contents;
    base::ReadFileToString(
        base::FilePath(test_file.value() + kMotwStreamSuffix), &motw_contents);

    // These warnings aren't displayed on successful test runs. They are there
    // so that we can check for deviations in behavior during manual testing.
    if (!motw_contents.empty()) {
      LOG(WARNING) << "Unexpected zone marker for file " << test_file.value()
                   << " Source URL:" << source_url;
      if (motw_contents != kMotwForInternetZone)
        LOG(WARNING) << "Zone marker contents: " << motw_contents;
    }

    base::DeleteFile(test_file, false);
  }

  // Bucket 1 is SUCCESS_WITHOUT_MOTW.
  histogram_tester.ExpectUniqueSample("Download.AttachmentServices.Result", 1,
                                      arraysize(kLocalSourceURLs));
}

// A file downloaded from the internet should be annotated with .. something.
// The specific zone assigned to our dummy source URL depends on the local
// configuration. But no sane configuration should be treating the dummy URL as
// a trusted source for anything.
TEST(QuarantineWinTest, DownloadedFile_DependsOnLocalConfig) {
  base::HistogramTester histogram_tester;
  base::ScopedTempDir test_dir;
  ASSERT_TRUE(test_dir.CreateUniqueTempDir());
  base::FilePath test_file = test_dir.GetPath().AppendASCII("foo.exe");

  for (const char* source_url : kUntrustedURLs) {
    SCOPED_TRACE(::testing::Message() << "Trying URL " << source_url);
    ASSERT_EQ(static_cast<int>(arraysize(kTestData)),
              base::WriteFile(test_file, kTestData, arraysize(kTestData)));
    EXPECT_EQ(
        QuarantineFileResult::OK,
        QuarantineFile(test_file, GURL(source_url), GURL(), kDummyClientGuid));
    std::string motw_contents;
    ASSERT_TRUE(base::ReadFileToString(
        base::FilePath(test_file.value() + kMotwStreamSuffix), &motw_contents));
    // The actual assigned zone could be anything. So only testing that there is
    // a zone annotation.
    EXPECT_FALSE(motw_contents.empty());

    // These warnings aren't displayed on successful test runs. They are there
    // so that we can check for deviations in behavior during manual testing.
    if (motw_contents != kMotwForInternetZone)
      LOG(WARNING) << "Unexpected zone marker: " << motw_contents;
    base::DeleteFile(test_file, false);
  }

  // Bucket 0 is SUCCESS_WITH_MOTW.
  histogram_tester.ExpectUniqueSample("Download.AttachmentServices.Result", 0,
                                      arraysize(kUntrustedURLs));
}

TEST(QuarantineWinTest, UnsafeReferrer_DependsOnLocalConfig) {
  base::HistogramTester histogram_tester;
  base::ScopedTempDir test_dir;
  ASSERT_TRUE(test_dir.CreateUniqueTempDir());
  base::FilePath test_file = test_dir.GetPath().AppendASCII("foo.exe");

  std::vector<std::string> unsafe_referrers(std::begin(kUntrustedURLs),
                                            std::end(kUntrustedURLs));

  std::string huge_referrer = "http://example.com/";
  huge_referrer.append(INTERNET_MAX_URL_LENGTH * 2, 'a');
  unsafe_referrers.push_back(huge_referrer);

  for (const auto referrer_url : unsafe_referrers) {
    SCOPED_TRACE(::testing::Message() << "Trying URL " << referrer_url);
    ASSERT_EQ(static_cast<int>(arraysize(kTestData)),
              base::WriteFile(test_file, kTestData, arraysize(kTestData)));
    EXPECT_EQ(QuarantineFileResult::OK,
              QuarantineFile(test_file, GURL("http://example.com/good"),
                             GURL(referrer_url), kDummyClientGuid));
    std::string motw_contents;
    ASSERT_TRUE(base::ReadFileToString(
        base::FilePath(test_file.value() + kMotwStreamSuffix), &motw_contents));
    // The actual assigned zone could be anything. So only testing that there is
    // a zone annotation.
    EXPECT_FALSE(motw_contents.empty());

    // These warnings aren't displayed on successful test runs. They are there
    // so that we can check for deviations in behavior during manual testing.
    if (motw_contents != kMotwForInternetZone)
      LOG(WARNING) << "Unexpected zone marker: " << motw_contents;
    base::DeleteFile(test_file, false);
  }

  // Bucket 0 is SUCCESS_WITH_MOTW.
  histogram_tester.ExpectUniqueSample("Download.AttachmentServices.Result", 0,
                                      unsafe_referrers.size());
}

// An empty source URL should result in a file that's treated the same as one
// downloaded from the internet.
TEST(QuarantineWinTest, EmptySource_DependsOnLocalConfig) {
  base::HistogramTester histogram_tester;
  base::ScopedTempDir test_dir;
  ASSERT_TRUE(test_dir.CreateUniqueTempDir());
  base::FilePath test_file = test_dir.GetPath().AppendASCII("foo.exe");
  ASSERT_EQ(static_cast<int>(arraysize(kTestData)),
            base::WriteFile(test_file, kTestData, arraysize(kTestData)));

  EXPECT_EQ(QuarantineFileResult::OK,
            QuarantineFile(test_file, GURL(), GURL(), kDummyClientGuid));
  std::string motw_contents;
  ASSERT_TRUE(base::ReadFileToString(
      base::FilePath(test_file.value() + kMotwStreamSuffix), &motw_contents));
  // The actual assigned zone could be anything. So only testing that there is a
  // zone annotation.
  EXPECT_FALSE(motw_contents.empty());

  // Bucket 0 is SUCCESS_WITH_MOTW.
  histogram_tester.ExpectUniqueSample("Download.AttachmentServices.Result", 0,
                                      1);
}

// Empty files aren't passed to AVScanFile. They are instead marked manually. If
// the file is passed to AVScanFile, then there wouldn't be a MOTW attached to
// it and the test would fail.
TEST(QuarantineWinTest, EmptyFile) {
  base::HistogramTester histogram_tester;
  base::ScopedTempDir test_dir;
  ASSERT_TRUE(test_dir.CreateUniqueTempDir());
  base::FilePath test_file = test_dir.GetPath().AppendASCII("foo.exe");
  ASSERT_EQ(0, base::WriteFile(test_file, "", 0u));

  EXPECT_EQ(QuarantineFileResult::OK,
            QuarantineFile(test_file, net::FilePathToFileURL(test_file), GURL(),
                           kDummyClientGuid));
  std::string motw_contents;
  ASSERT_TRUE(base::ReadFileToString(
      base::FilePath(test_file.value() + kMotwStreamSuffix), &motw_contents));
  EXPECT_STREQ(kMotwForInternetZone, motw_contents.c_str());

  // Attachment services shouldn't have been invoked at all.
  histogram_tester.ExpectTotalCount("Download.AttachmentServices.Result", 0);
}

// If there is no client GUID supplied to the QuarantineFile() call, then rather
// than invoking AVScanFile, the MOTW will be applied manually.  If the file is
// passed to AVScanFile, then there wouldn't be a MOTW attached to it and the
// test would fail.
TEST(QuarantineWinTest, NoClientGuid) {
  base::ScopedTempDir test_dir;
  ASSERT_TRUE(test_dir.CreateUniqueTempDir());
  base::FilePath test_file = test_dir.GetPath().AppendASCII("foo.exe");
  ASSERT_EQ(static_cast<int>(arraysize(kTestData)),
            base::WriteFile(test_file, kTestData, arraysize(kTestData)));

  EXPECT_EQ(QuarantineFileResult::OK,
            QuarantineFile(test_file, net::FilePathToFileURL(test_file), GURL(),
                           std::string()));
  std::string motw_contents;
  ASSERT_TRUE(base::ReadFileToString(
      base::FilePath(test_file.value() + kMotwStreamSuffix), &motw_contents));
  EXPECT_STREQ(kMotwForInternetZone, motw_contents.c_str());
}

// URLs longer than INTERNET_MAX_URL_LENGTH are known to break URLMon. Such a
// URL, when used as a source URL shouldn't break QuarantineFile() which should
// mark the file as being from the internet zone as a safe fallback.
TEST(QuarantineWinTest, SuperLongURL) {
  base::ScopedTempDir test_dir;
  ASSERT_TRUE(test_dir.CreateUniqueTempDir());
  base::FilePath test_file = test_dir.GetPath().AppendASCII("foo.exe");
  ASSERT_EQ(static_cast<int>(arraysize(kTestData)),
            base::WriteFile(test_file, kTestData, arraysize(kTestData)));

  std::string source_url("http://example.com/");
  source_url.append(INTERNET_MAX_URL_LENGTH * 2, 'a');
  EXPECT_EQ(QuarantineFileResult::OK,
            QuarantineFile(test_file, GURL(source_url), GURL(), std::string()));

  std::string motw_contents;
  ASSERT_TRUE(base::ReadFileToString(
      base::FilePath(test_file.value() + kMotwStreamSuffix), &motw_contents));
  EXPECT_STREQ(kMotwForInternetZone, motw_contents.c_str());
}

}  // namespace download
