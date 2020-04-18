// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/file_handlers/mime_util.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/run_loop.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/extensions_test.h"
#include "storage/browser/test/test_file_system_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {
namespace app_file_handler_util {
namespace {

const char kOrigin[] = "chrome-extension://cmalghjoncmjoeakimpfhojhpgemgaje";
const char kJPEGExtensionFilePath[] = "/fake/path/foo.jpg";
const char kJPEGExtensionUpperCaseFilePath[] = "/fake/path/FOO.JPG";

// Saves the returned mime type to a variable.
void OnMimeTypeResult(std::string* output, const std::string& mime_type) {
  *output = mime_type;
}

// Saves returned mime types to a vector.
void OnMimeTypesCollected(
    std::vector<std::string>* output,
    std::unique_ptr<std::vector<std::string>> mime_types) {
  *output = *mime_types;
}

// Creates a native local file system URL for a local path.
storage::FileSystemURL CreateNativeLocalFileSystemURL(
    storage::FileSystemContext* context,
    const base::FilePath local_path) {
  return context->CreateCrackedFileSystemURL(
      GURL(kOrigin), storage::kFileSystemTypeNativeLocal, local_path);
}

}  // namespace

class FileHandlersMimeUtilTest : public ExtensionsTest {
 protected:
  FileHandlersMimeUtilTest() {}
  ~FileHandlersMimeUtilTest() override {}

  void SetUp() override {
    ExtensionsTest::SetUp();
    file_system_context_ = content::CreateFileSystemContextForTesting(
        NULL, browser_context()->GetPath());

    base::FilePath temp_filename;
    EXPECT_TRUE(base::CreateTemporaryFile(&temp_filename));
    const std::string kSampleContent = "<html><body></body></html>";
    EXPECT_EQ(static_cast<int>(kSampleContent.size()),
              base::WriteFile(temp_filename, kSampleContent.c_str(),
                              kSampleContent.size()));
    // File path must end in .html to avoid relying upon MIME-sniffing, which
    // is disabled for HTML files delivered from file:// URIs.
    html_mime_file_path_ =
        temp_filename.AddExtension(FILE_PATH_LITERAL(".html"));
    EXPECT_TRUE(base::Move(temp_filename, html_mime_file_path_));
  }

  ExtensionsAPIClient extensions_api_client_;
  scoped_refptr<storage::FileSystemContext> file_system_context_;
  base::FilePath html_mime_file_path_;
};

TEST_F(FileHandlersMimeUtilTest, GetMimeTypeForLocalPath) {
  {
    std::string result;
    GetMimeTypeForLocalPath(browser_context(), base::FilePath::FromUTF8Unsafe(
                                                   kJPEGExtensionFilePath),
                            base::Bind(&OnMimeTypeResult, &result));
    content::RunAllTasksUntilIdle();
    EXPECT_EQ("image/jpeg", result);
  }

  {
    std::string result;
    GetMimeTypeForLocalPath(
        browser_context(),
        base::FilePath::FromUTF8Unsafe(kJPEGExtensionUpperCaseFilePath),
        base::Bind(&OnMimeTypeResult, &result));
    content::RunAllTasksUntilIdle();
    EXPECT_EQ("image/jpeg", result);
  }

  {
    std::string result;
    GetMimeTypeForLocalPath(browser_context(), html_mime_file_path_,
                            base::Bind(&OnMimeTypeResult, &result));
    content::RunAllTasksUntilIdle();
    EXPECT_EQ("text/html", result);
  }
}

TEST_F(FileHandlersMimeUtilTest, MimeTypeCollector_ForURLs) {
  MimeTypeCollector collector(browser_context());

  std::vector<storage::FileSystemURL> urls;
  urls.push_back(CreateNativeLocalFileSystemURL(
      file_system_context_.get(),
      base::FilePath::FromUTF8Unsafe(kJPEGExtensionFilePath)));
  urls.push_back(CreateNativeLocalFileSystemURL(
      file_system_context_.get(),
      base::FilePath::FromUTF8Unsafe(kJPEGExtensionUpperCaseFilePath)));
  urls.push_back(CreateNativeLocalFileSystemURL(file_system_context_.get(),
                                                html_mime_file_path_));

  std::vector<std::string> result;
  collector.CollectForURLs(urls, base::Bind(&OnMimeTypesCollected, &result));
  content::RunAllTasksUntilIdle();

  ASSERT_EQ(3u, result.size());
  EXPECT_EQ("image/jpeg", result[0]);
  EXPECT_EQ("image/jpeg", result[1]);
  EXPECT_EQ("text/html", result[2]);
}

TEST_F(FileHandlersMimeUtilTest, MimeTypeCollector_ForLocalPaths) {
  MimeTypeCollector collector(browser_context());

  std::vector<base::FilePath> local_paths;
  local_paths.push_back(base::FilePath::FromUTF8Unsafe(kJPEGExtensionFilePath));
  local_paths.push_back(
      base::FilePath::FromUTF8Unsafe(kJPEGExtensionUpperCaseFilePath));
  local_paths.push_back(html_mime_file_path_);

  std::vector<std::string> result;
  collector.CollectForLocalPaths(local_paths,
                                 base::Bind(&OnMimeTypesCollected, &result));
  content::RunAllTasksUntilIdle();

  ASSERT_EQ(3u, result.size());
  EXPECT_EQ("image/jpeg", result[0]);
  EXPECT_EQ("image/jpeg", result[1]);
  EXPECT_EQ("text/html", result[2]);
}

}  // namespace app_file_handler_util
}  // namespace extensions
