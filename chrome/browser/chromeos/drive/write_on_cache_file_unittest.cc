// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/drive/write_on_cache_file.h"

#include "base/bind.h"
#include "components/drive/chromeos/dummy_file_system.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {

namespace {

const base::FilePath::CharType kDrivePath[] =
    FILE_PATH_LITERAL("drive/root/file.txt");
const base::FilePath::CharType kInvalidPath[] =
    FILE_PATH_LITERAL("drive/invalid/path");
const base::FilePath::CharType kLocalPath[] =
    FILE_PATH_LITERAL("/tmp/local.txt");

class TestFileSystem : public DummyFileSystem {
 public:
  TestFileSystem() : num_closed_(0) {
  }

  int num_closed() const { return num_closed_; }

  // Mimics OpenFile. It fails if the |file_path| points to a hosted document.
  void OpenFile(const base::FilePath& file_path,
                OpenMode open_mode,
                const std::string& mime_type,
                const OpenFileCallback& callback) override {
    EXPECT_EQ(OPEN_OR_CREATE_FILE, open_mode);

    // Emulate a case of opening a hosted document.
    if (file_path == base::FilePath(kInvalidPath)) {
      callback.Run(FILE_ERROR_INVALID_OPERATION, base::FilePath(),
                   base::Closure());
      return;
    }

    callback.Run(FILE_ERROR_OK, base::FilePath(kLocalPath),
                 base::Bind(&TestFileSystem::CloseFile,
                            base::Unretained(this)));
  }

 private:

  void CloseFile() {
    ++num_closed_;
  }

  int num_closed_;
};

}  // namespace

TEST(WriteOnCacheFileTest, PrepareFileForWritingSuccess) {
  content::TestBrowserThreadBundle thread_bundle;
  TestFileSystem test_file_system;

  FileError error = FILE_ERROR_FAILED;
  base::FilePath path;
  // The file should successfully be opened.
  WriteOnCacheFile(
      &test_file_system,
      base::FilePath(kDrivePath),
      std::string(),  // mime_type
      google_apis::test_util::CreateCopyResultCallback(&error, &path));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(kLocalPath, path.value());

  // Make sure that the file is actually closed.
  EXPECT_EQ(1, test_file_system.num_closed());
}

TEST(WriteOnCacheFileTest, PrepareFileForWritingCreateFail) {
  content::TestBrowserThreadBundle thread_bundle;
  TestFileSystem test_file_system;

  FileError error = FILE_ERROR_FAILED;
  base::FilePath path;
  // Access to kInvalidPath should fail, and FileWriteHelper should not try to
  // open or close the file.
  WriteOnCacheFile(
      &test_file_system,
      base::FilePath(kInvalidPath),
      std::string(),  // mime_type
      google_apis::test_util::CreateCopyResultCallback(&error, &path));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_INVALID_OPERATION, error);
  EXPECT_TRUE(path.empty());
}

}   // namespace drive
