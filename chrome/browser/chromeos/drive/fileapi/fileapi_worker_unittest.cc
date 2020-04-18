// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/drive/fileapi/fileapi_worker.h"

#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_util.h"
#include "components/drive/chromeos/dummy_file_system.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace fileapi_internal {
namespace {

// Increments |num_called| for checking how many times the closure is called.
void Increment(int* num_called) {
  ++*num_called;
}

// Returns the |instance| as is.
FileSystemInterface* GetFileSystem(FileSystemInterface* instance) {
  return instance;
}

// A test file system that always returns |local_file_path|. For testing
// purpose, it checks if |open_mode| is the expected value, and record if the
// close callback is called.
class TestFileSystemForOpenFile : public DummyFileSystem {
 public:
  TestFileSystemForOpenFile(const base::FilePath& local_file_path,
                            OpenMode expected_open_mode)
      : local_file_path_(local_file_path),
        expected_open_mode_(expected_open_mode),
        closed_(false) {
  }

  void OpenFile(const base::FilePath& file_path,
                OpenMode open_mode,
                const std::string& mime_type,
                const drive::OpenFileCallback& callback) override {
    EXPECT_EQ(expected_open_mode_, open_mode);

    callback.Run(
        FILE_ERROR_OK,
        local_file_path_,
        base::Bind(&TestFileSystemForOpenFile::Close, base::Unretained(this)));
  }

  void Close() {
    closed_ = true;
  }

  bool closed() const { return closed_; }

 private:
  const base::FilePath local_file_path_;
  const OpenMode expected_open_mode_;
  bool closed_;
};

// Helper function of testing OpenFile() for write access. It checks that the
// file handle correctly writes to the expected file.
void VerifyWrite(int64_t expected_size,
                 const base::FilePath& expected_written_path,
                 const std::string& write_data,
                 base::File file,
                 const base::Closure& close_callback) {
  // Check that the file was properly opened.
  EXPECT_TRUE(file.IsValid());
  EXPECT_FALSE(close_callback.is_null());

  // Check that the file has the expected length (i.e., truncated or not)
  base::File::Info info;
  EXPECT_TRUE(file.GetInfo(&info));
  EXPECT_EQ(expected_size, info.size);

  // Write some data.
  const int data_size = static_cast<int>(write_data.size());
  EXPECT_EQ(data_size, file.Write(0, write_data.c_str(), data_size));
  EXPECT_TRUE(file.SetLength(data_size));

  // Close.
  file.Close();
  close_callback.Run();

  // Checks that the written content goes to |expected_written_path|. I.e.,
  // the |file| handle is pointing to the file.
  std::string written;
  EXPECT_TRUE(base::ReadFileToString(expected_written_path, &written));
  EXPECT_EQ(write_data, written);
}

// Helper function of testing OpenFile() for read access. It checks that the
// file is readable and contains |expected_data|.
void VerifyRead(const std::string& expected_data,
                base::File file,
                const base::Closure& close_callback) {
  // Check that the file was properly opened.
  EXPECT_TRUE(file.IsValid());
  EXPECT_FALSE(close_callback.is_null());

  // Check that the file has the expected content.
  const int data_size = static_cast<int>(expected_data.size());
  base::File::Info info;
  EXPECT_TRUE(file.GetInfo(&info));
  EXPECT_EQ(data_size, info.size);

  std::vector<char> buffer(data_size);
  EXPECT_EQ(data_size, file.Read(0, buffer.data(), data_size));
  EXPECT_EQ(expected_data, std::string(buffer.begin(), buffer.end()));

  // Close.
  file.Close();
  close_callback.Run();
}

}  // namespace

class FileApiWorkerTest : public testing::Test {
 private:
  content::TestBrowserThreadBundle thread_bundle_;
};

TEST_F(FileApiWorkerTest, RunFileSystemCallbackSuccess) {
  DummyFileSystem dummy_file_system;

  FileSystemInterface* file_system = NULL;
  RunFileSystemCallback(
      base::Bind(&GetFileSystem, &dummy_file_system),
      google_apis::test_util::CreateCopyResultCallback(&file_system),
      base::Closure());

  EXPECT_EQ(&dummy_file_system, file_system);
}

TEST_F(FileApiWorkerTest, RunFileSystemCallbackFail) {
  FileSystemInterface* file_system = NULL;

  // Make sure on_error_callback is called if file_system_getter returns NULL.
  int num_called = 0;
  RunFileSystemCallback(
      base::Bind(&GetFileSystem, static_cast<FileSystemInterface*>(NULL)),
      google_apis::test_util::CreateCopyResultCallback(&file_system),
      base::Bind(&Increment, &num_called));
  EXPECT_EQ(1, num_called);

  // Just make sure this null |on_error_callback| doesn't cause a crash.
  RunFileSystemCallback(
      base::Bind(&GetFileSystem, static_cast<FileSystemInterface*>(NULL)),
      google_apis::test_util::CreateCopyResultCallback(&file_system),
      base::Closure());
}

TEST_F(FileApiWorkerTest, OpenFileForCreateWrite) {
  const base::FilePath kDummyPath = base::FilePath::FromUTF8Unsafe("whatever");
  const std::string kWriteData = "byebye";

  base::FilePath temp_path;
  base::CreateTemporaryFile(&temp_path);

  // CREATE => CREATE (fails if file exists.)
  TestFileSystemForOpenFile file_system(temp_path, CREATE_FILE);
  const int64_t kExpectedSize = 0;

  OpenFile(kDummyPath,
           base::File::FLAG_CREATE | base::File::FLAG_WRITE,
           base::Bind(&VerifyWrite, kExpectedSize, temp_path, kWriteData),
           &file_system);
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(file_system.closed());
}

TEST_F(FileApiWorkerTest, OpenFileForOpenAlwaysWrite) {
  const base::FilePath kDummyPath = base::FilePath::FromUTF8Unsafe("whatever");
  const std::string kWriteData = "byebye";
  const std::string kInitialData = "hello";

  base::FilePath temp_path;
  base::CreateTemporaryFile(&temp_path);
  google_apis::test_util::WriteStringToFile(temp_path, kInitialData);

  // OPEN_ALWAYS => OPEN_OR_CREATE (success whether file exists or not.)
  // No truncation should take place.
  TestFileSystemForOpenFile file_system(temp_path, OPEN_OR_CREATE_FILE);
  const int64_t kExpectedSize = static_cast<int64_t>(kInitialData.size());

  OpenFile(kDummyPath,
           base::File::FLAG_OPEN_ALWAYS | base::File::FLAG_WRITE,
           base::Bind(&VerifyWrite, kExpectedSize, temp_path, kWriteData),
           &file_system);
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(file_system.closed());
}

TEST_F(FileApiWorkerTest, OpenFileForOpenTruncatedWrite) {
  const base::FilePath kDummyPath = base::FilePath::FromUTF8Unsafe("whatever");
  const std::string kInitialData = "hello";
  const std::string kWriteData = "byebye";

  base::FilePath temp_path;
  base::CreateTemporaryFile(&temp_path);
  google_apis::test_util::WriteStringToFile(temp_path, kInitialData);

  // OPEN_TRUNCATED => OPEN (failure when the file did not exist.)
  // It should truncate the file before passing to the callback.
  TestFileSystemForOpenFile file_system(temp_path, OPEN_FILE);
  const int64_t kExpectedSize = 0;

  OpenFile(kDummyPath,
           base::File::FLAG_OPEN_TRUNCATED | base::File::FLAG_WRITE,
           base::Bind(&VerifyWrite, kExpectedSize, temp_path, kWriteData),
           &file_system);
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(file_system.closed());
}

TEST_F(FileApiWorkerTest, OpenFileForOpenCreateAlwaysWrite) {
  const base::FilePath kDummyPath = base::FilePath::FromUTF8Unsafe("whatever");
  const std::string kInitialData = "hello";
  const std::string kWriteData = "byebye";

  base::FilePath temp_path;
  base::CreateTemporaryFile(&temp_path);
  google_apis::test_util::WriteStringToFile(temp_path, kInitialData);

  // CREATE_ALWAYS => OPEN_OR_CREATE (success whether file exists or not.)
  // It should truncate the file before passing to the callback.
  TestFileSystemForOpenFile file_system(temp_path, OPEN_OR_CREATE_FILE);
  const int64_t kExpectedSize = 0;

  OpenFile(kDummyPath,
           base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE,
           base::Bind(&VerifyWrite, kExpectedSize, temp_path, kWriteData),
           &file_system);
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(file_system.closed());
}

TEST_F(FileApiWorkerTest, OpenFileForOpenRead) {
  const base::FilePath kDummyPath = base::FilePath::FromUTF8Unsafe("whatever");
  const std::string kInitialData = "hello";

  base::FilePath temp_path;
  base::CreateTemporaryFile(&temp_path);
  google_apis::test_util::WriteStringToFile(temp_path, kInitialData);

  // OPEN => OPEN (failure when the file did not exist.)
  TestFileSystemForOpenFile file_system(temp_path, OPEN_FILE);

  OpenFile(kDummyPath,
           base::File::FLAG_OPEN | base::File::FLAG_READ,
           base::Bind(&VerifyRead, kInitialData),
           &file_system);
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(file_system.closed());
}

}  // namespace fileapi_internal
}  // namespace drive
