// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/local_file_reader.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/rand_util.h"
#include "base/threading/thread.h"
#include "components/drive/chromeos/drive_test_util.h"
#include "google_apis/drive/test_util.h"
#include "net/base/io_buffer.h"
#include "net/base/test_completion_callback.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace util {
namespace {

// An adapter to use test_util::ReadAllData.
class LocalFileReaderAdapter {
 public:
  explicit LocalFileReaderAdapter(LocalFileReader* reader) : reader_(reader) {}
  int Read(net::IOBuffer* buffer,
           int buffer_length,
           const net::CompletionCallback& callback) {
    reader_->Read(buffer, buffer_length, callback);
    // As LocalFileReader::Read always works asynchronously,
    // return ERR_IO_PENDING.
    return net::ERR_IO_PENDING;
  }

 private:
  LocalFileReader* reader_;
};

}  // namespace

class LocalFileReaderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    worker_thread_.reset(new base::Thread("LocalFileReaderTest"));
    ASSERT_TRUE(worker_thread_->Start());
    file_reader_.reset(
        new LocalFileReader(worker_thread_->task_runner().get()));
  }

  base::MessageLoop message_loop_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<base::Thread> worker_thread_;
  std::unique_ptr<LocalFileReader> file_reader_;
};

TEST_F(LocalFileReaderTest, NonExistingFile) {
  const base::FilePath kTestFile =
      temp_dir_.GetPath().AppendASCII("non-existing");

  net::TestCompletionCallback callback;
  file_reader_->Open(kTestFile, 0, callback.callback());
  EXPECT_EQ(net::ERR_FILE_NOT_FOUND, callback.WaitForResult());
}

TEST_F(LocalFileReaderTest, FullRead) {
  base::FilePath test_file;
  std::string expected_content;
  ASSERT_TRUE(google_apis::test_util::CreateFileOfSpecifiedSize(
      temp_dir_.GetPath(), 1024, &test_file, &expected_content));

  net::TestCompletionCallback callback;
  file_reader_->Open(test_file, 0, callback.callback());
  ASSERT_EQ(net::OK, callback.WaitForResult());

  LocalFileReaderAdapter adapter(file_reader_.get());
  std::string content;
  ASSERT_EQ(net::OK, test_util::ReadAllData(&adapter, &content));
  EXPECT_EQ(expected_content, content);
}

TEST_F(LocalFileReaderTest, OpenWithOffset) {
  base::FilePath test_file;
  std::string expected_content;
  ASSERT_TRUE(google_apis::test_util::CreateFileOfSpecifiedSize(
      temp_dir_.GetPath(), 1024, &test_file, &expected_content));

  size_t offset = expected_content.size() / 2;
  expected_content.erase(0, offset);

  net::TestCompletionCallback callback;
  file_reader_->Open(test_file, static_cast<int64_t>(offset),
                     callback.callback());
  ASSERT_EQ(net::OK, callback.WaitForResult());

  LocalFileReaderAdapter adapter(file_reader_.get());
  std::string content;
  ASSERT_EQ(net::OK, test_util::ReadAllData(&adapter, &content));
  EXPECT_EQ(expected_content, content);
}

}  // namespace util
}  // namespace drive
