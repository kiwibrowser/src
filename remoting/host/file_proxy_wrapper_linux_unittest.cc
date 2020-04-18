// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/file_proxy_wrapper.h"

#include <memory>
#include <queue>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/containers/queue.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "net/base/io_buffer.h"
#include "remoting/base/compound_buffer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kTestFilename[] = "test-file.txt";
constexpr char kTestFilenameSecondary[] = "test-file (1).txt";
const std::string& kTestDataOne = "this is the first test string";
const std::string& kTestDataTwo = "this is the second test string";
const std::string& kTestDataThree = "this is the third test string";

std::unique_ptr<remoting::CompoundBuffer> ToBuffer(const std::string& data) {
  std::unique_ptr<remoting::CompoundBuffer> buffer =
      std::make_unique<remoting::CompoundBuffer>();
  buffer->Append(new net::WrappedIOBuffer(data.data()), data.size());
  return buffer;
}

}  // namespace

namespace remoting {

class FileProxyWrapperLinuxTest : public testing::Test {
 public:
  FileProxyWrapperLinuxTest();
  ~FileProxyWrapperLinuxTest() override;

  // testing::Test implementation.
  void SetUp() override;
  void TearDown() override;

  const base::FilePath& TestDir() const { return dir_.GetPath(); }
  const base::FilePath TestFilePath() const {
    return dir_.GetPath().Append(kTestFilename);
  }

  void StatusCallback(
      FileProxyWrapper::State state,
      base::Optional<protocol::FileTransferResponse_ErrorCode> error);
  void OpenFileCallback(int64_t filesize);
  void ReadChunkCallback(std::unique_ptr<std::vector<char>> chunk);

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::ScopedTempDir dir_;

  std::unique_ptr<FileProxyWrapper> file_proxy_wrapper_;
  base::Optional<protocol::FileTransferResponse_ErrorCode> error_;
  FileProxyWrapper::State final_state_;
  bool done_callback_succeeded_;

  base::queue<std::vector<char>> read_chunks_;
  int64_t read_filesize_;
};

FileProxyWrapperLinuxTest::FileProxyWrapperLinuxTest()
    : scoped_task_environment_(
          base::test::ScopedTaskEnvironment::MainThreadType::DEFAULT,
          base::test::ScopedTaskEnvironment::ExecutionMode::QUEUED) {}
FileProxyWrapperLinuxTest::~FileProxyWrapperLinuxTest() = default;

void FileProxyWrapperLinuxTest::SetUp() {
  ASSERT_TRUE(dir_.CreateUniqueTempDir());

  file_proxy_wrapper_ = FileProxyWrapper::Create();
  file_proxy_wrapper_->Init(base::BindOnce(
      &FileProxyWrapperLinuxTest::StatusCallback, base::Unretained(this)));

  error_ = base::Optional<protocol::FileTransferResponse_ErrorCode>();
  final_state_ = FileProxyWrapper::kUninitialized;
  done_callback_succeeded_ = false;

  read_chunks_ = base::queue<std::vector<char>>();
  read_filesize_ = 0;
}

void FileProxyWrapperLinuxTest::TearDown() {
  file_proxy_wrapper_.reset();
}

void FileProxyWrapperLinuxTest::StatusCallback(
    FileProxyWrapper::State state,
    base::Optional<protocol::FileTransferResponse_ErrorCode> error) {
  final_state_ = state;
  error_ = error;
  done_callback_succeeded_ = !error_.has_value();
}

void FileProxyWrapperLinuxTest::OpenFileCallback(int64_t filesize) {
  read_filesize_ = filesize;
}

void FileProxyWrapperLinuxTest::ReadChunkCallback(
    std::unique_ptr<std::vector<char>> chunk) {
  read_chunks_.push(*chunk);
}

// Verifies that FileProxyWrapper can write three chunks to a file without
// throwing any errors.
TEST_F(FileProxyWrapperLinuxTest, WriteThreeChunks) {
  file_proxy_wrapper_->CreateFile(TestDir(), kTestFilename);
  file_proxy_wrapper_->WriteChunk(ToBuffer(kTestDataOne));
  file_proxy_wrapper_->WriteChunk(ToBuffer(kTestDataTwo));
  file_proxy_wrapper_->WriteChunk(ToBuffer(kTestDataThree));
  file_proxy_wrapper_->Close();
  scoped_task_environment_.RunUntilIdle();

  ASSERT_FALSE(error_);
  ASSERT_EQ(final_state_, FileProxyWrapper::kClosed);
  ASSERT_TRUE(done_callback_succeeded_);

  std::string actual_file_data;
  ASSERT_TRUE(base::ReadFileToString(TestFilePath(), &actual_file_data));
  ASSERT_TRUE(kTestDataOne + kTestDataTwo + kTestDataThree == actual_file_data);
}

// Verifies that calling Cancel() deletes any temporary or destination files.
TEST_F(FileProxyWrapperLinuxTest, CancelDeletesFiles) {
  file_proxy_wrapper_->CreateFile(TestDir(), kTestFilename);
  file_proxy_wrapper_->WriteChunk(ToBuffer(kTestDataOne));
  scoped_task_environment_.RunUntilIdle();

  file_proxy_wrapper_->Cancel();
  file_proxy_wrapper_.reset();
  scoped_task_environment_.RunUntilIdle();

  ASSERT_TRUE(base::IsDirectoryEmpty(TestDir()));
}

// Verifies that FileProxyWrapper will write to a file named "file (1).txt" when
// "file.txt" already exists.
TEST_F(FileProxyWrapperLinuxTest, FileAlreadyExists) {
  WriteFile(TestFilePath(), kTestDataOne.data(), kTestDataOne.size());

  file_proxy_wrapper_->CreateFile(TestDir(), kTestFilename);
  file_proxy_wrapper_->WriteChunk(ToBuffer(kTestDataTwo));
  file_proxy_wrapper_->Close();
  scoped_task_environment_.RunUntilIdle();

  std::string actual_file_data;
  base::FilePath secondary_filepath = TestDir().Append(kTestFilenameSecondary);
  ASSERT_TRUE(base::ReadFileToString(secondary_filepath, &actual_file_data));
  ASSERT_STREQ(kTestDataTwo.data(), actual_file_data.data());

  ASSERT_FALSE(error_);
  ASSERT_EQ(final_state_, FileProxyWrapper::kClosed);
}

// Verifies that FileProxyWrapper can read chunks from a file.
TEST_F(FileProxyWrapperLinuxTest, ReadThreeChunks) {
  std::string test_data = kTestDataOne + kTestDataTwo + kTestDataThree;
  WriteFile(TestFilePath(), test_data.data(), test_data.size());

  file_proxy_wrapper_->OpenFile(
      TestFilePath(), base::Bind(&FileProxyWrapperLinuxTest::OpenFileCallback,
                                 base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();
  ASSERT_FALSE(error_);
  ASSERT_EQ(static_cast<uint64_t>(read_filesize_), test_data.size());

  file_proxy_wrapper_->ReadChunk(
      kTestDataOne.size(),
      base::BindOnce(&FileProxyWrapperLinuxTest::ReadChunkCallback,
                     base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();

  file_proxy_wrapper_->ReadChunk(
      kTestDataTwo.size(),
      base::BindOnce(&FileProxyWrapperLinuxTest::ReadChunkCallback,
                     base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();

  file_proxy_wrapper_->ReadChunk(
      kTestDataThree.size(),
      base::BindOnce(&FileProxyWrapperLinuxTest::ReadChunkCallback,
                     base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();

  file_proxy_wrapper_->Close();
  scoped_task_environment_.RunUntilIdle();

  base::queue<std::vector<char>> expected_read_chunks;
  expected_read_chunks.push(
      std::vector<char>(kTestDataOne.begin(), kTestDataOne.end()));
  expected_read_chunks.push(
      std::vector<char>(kTestDataTwo.begin(), kTestDataTwo.end()));
  expected_read_chunks.push(
      std::vector<char>(kTestDataThree.begin(), kTestDataThree.end()));
  ASSERT_FALSE(error_);

  ASSERT_EQ(expected_read_chunks.size(), read_chunks_.size());
  while (!expected_read_chunks.empty()) {
    ASSERT_EQ(expected_read_chunks.front(), read_chunks_.front());
    expected_read_chunks.pop();
    read_chunks_.pop();
  }
}

// Verifies that FileProxyWrapper fails to open a file for reading if the file
// doesn't exist.
TEST_F(FileProxyWrapperLinuxTest, FileDoesntExist) {
  file_proxy_wrapper_->OpenFile(
      TestFilePath(), base::Bind(&FileProxyWrapperLinuxTest::OpenFileCallback,
                                 base::Unretained(this)));
  scoped_task_environment_.RunUntilIdle();

  ASSERT_EQ(error_, protocol::FileTransferResponse_ErrorCode_FILE_IO_ERROR);
}

}  // namespace remoting
