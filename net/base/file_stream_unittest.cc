// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/file_stream.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/message_loop/message_loop_current.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/log/test_net_log.h"
#include "net/test/gtest_util.h"
#include "net/test/test_with_scoped_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

using net::test::IsError;
using net::test::IsOk;

#if defined(OS_ANDROID)
#include "base/test/test_file_util.h"
#endif

namespace net {

namespace {

constexpr char kTestData[] = "0123456789";
constexpr int kTestDataSize = base::size(kTestData) - 1;

// Creates an IOBufferWithSize that contains the kTestDataSize.
IOBufferWithSize* CreateTestDataBuffer() {
  IOBufferWithSize* buf = new IOBufferWithSize(kTestDataSize);
  memcpy(buf->data(), kTestData, kTestDataSize);
  return buf;
}

}  // namespace

class FileStreamTest : public PlatformTest, public WithScopedTaskEnvironment {
 public:
  void SetUp() override {
    PlatformTest::SetUp();

    base::CreateTemporaryFile(&temp_file_path_);
    base::WriteFile(temp_file_path_, kTestData, kTestDataSize);
  }
  void TearDown() override {
    // FileStreamContexts must be asynchronously closed on the file task runner
    // before they can be deleted. Pump the RunLoop to avoid leaks.
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(base::DeleteFile(temp_file_path_, false));

    PlatformTest::TearDown();
  }

  const base::FilePath temp_file_path() const { return temp_file_path_; }

 private:
  base::FilePath temp_file_path_;
};

namespace {

TEST_F(FileStreamTest, OpenExplicitClose) {
  TestCompletionCallback callback;
  FileStream stream(base::ThreadTaskRunnerHandle::Get());
  int flags = base::File::FLAG_OPEN |
              base::File::FLAG_READ |
              base::File::FLAG_ASYNC;
  int rv = stream.Open(temp_file_path(), flags, callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
  EXPECT_THAT(callback.WaitForResult(), IsOk());
  EXPECT_TRUE(stream.IsOpen());
  EXPECT_THAT(stream.Close(callback.callback()), IsError(ERR_IO_PENDING));
  EXPECT_THAT(callback.WaitForResult(), IsOk());
  EXPECT_FALSE(stream.IsOpen());
}

TEST_F(FileStreamTest, OpenExplicitCloseOrphaned) {
  TestCompletionCallback callback;
  std::unique_ptr<FileStream> stream(
      new FileStream(base::ThreadTaskRunnerHandle::Get()));
  int flags = base::File::FLAG_OPEN | base::File::FLAG_READ |
              base::File::FLAG_ASYNC;
  int rv = stream->Open(temp_file_path(), flags, callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
  EXPECT_THAT(callback.WaitForResult(), IsOk());
  EXPECT_TRUE(stream->IsOpen());
  EXPECT_THAT(stream->Close(callback.callback()), IsError(ERR_IO_PENDING));
  stream.reset();
  // File isn't actually closed yet.
  base::RunLoop runloop;
  runloop.RunUntilIdle();
  // The file should now be closed, though the callback has not been called.
}

// Test the use of FileStream with a file handle provided at construction.
TEST_F(FileStreamTest, UseFileHandle) {
  int rv = 0;
  TestCompletionCallback callback;
  TestInt64CompletionCallback callback64;
  // 1. Test reading with a file handle.
  ASSERT_EQ(kTestDataSize,
            base::WriteFile(temp_file_path(), kTestData, kTestDataSize));
  int flags = base::File::FLAG_OPEN_ALWAYS | base::File::FLAG_READ |
              base::File::FLAG_ASYNC;
  base::File file(temp_file_path(), flags);

  // Seek to the beginning of the file and read.
  std::unique_ptr<FileStream> read_stream(
      new FileStream(std::move(file), base::ThreadTaskRunnerHandle::Get()));
  ASSERT_THAT(read_stream->Seek(0, callback64.callback()),
              IsError(ERR_IO_PENDING));
  ASSERT_EQ(0, callback64.WaitForResult());
  // Read into buffer and compare.
  scoped_refptr<IOBufferWithSize> read_buffer =
      new IOBufferWithSize(kTestDataSize);
  rv = read_stream->Read(read_buffer.get(), kTestDataSize, callback.callback());
  ASSERT_EQ(kTestDataSize, callback.GetResult(rv));
  ASSERT_EQ(0, memcmp(kTestData, read_buffer->data(), kTestDataSize));
  read_stream.reset();

  // 2. Test writing with a file handle.
  base::DeleteFile(temp_file_path(), false);
  flags = base::File::FLAG_OPEN_ALWAYS | base::File::FLAG_WRITE |
          base::File::FLAG_ASYNC;
  file.Initialize(temp_file_path(), flags);

  std::unique_ptr<FileStream> write_stream(
      new FileStream(std::move(file), base::ThreadTaskRunnerHandle::Get()));
  ASSERT_THAT(write_stream->Seek(0, callback64.callback()),
              IsError(ERR_IO_PENDING));
  ASSERT_EQ(0, callback64.WaitForResult());
  scoped_refptr<IOBufferWithSize> write_buffer = CreateTestDataBuffer();
  rv = write_stream->Write(write_buffer.get(), kTestDataSize,
                           callback.callback());
  ASSERT_EQ(kTestDataSize, callback.GetResult(rv));
  write_stream.reset();

  // Read into buffer and compare to make sure the handle worked fine.
  ASSERT_EQ(kTestDataSize,
            base::ReadFile(temp_file_path(), read_buffer->data(),
                           kTestDataSize));
  ASSERT_EQ(0, memcmp(kTestData, read_buffer->data(), kTestDataSize));
}

TEST_F(FileStreamTest, UseClosedStream) {
  int rv = 0;
  TestCompletionCallback callback;
  TestInt64CompletionCallback callback64;

  FileStream stream(base::ThreadTaskRunnerHandle::Get());

  EXPECT_FALSE(stream.IsOpen());

  // Try seeking...
  rv = stream.Seek(5, callback64.callback());
  EXPECT_THAT(callback64.GetResult(rv), IsError(ERR_UNEXPECTED));

  // Try reading...
  scoped_refptr<IOBufferWithSize> buf = new IOBufferWithSize(10);
  rv = stream.Read(buf.get(), buf->size(), callback.callback());
  EXPECT_THAT(callback.GetResult(rv), IsError(ERR_UNEXPECTED));
}

TEST_F(FileStreamTest, Read) {
  int64_t file_size;
  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));

  FileStream stream(base::ThreadTaskRunnerHandle::Get());
  int flags = base::File::FLAG_OPEN | base::File::FLAG_READ |
              base::File::FLAG_ASYNC;
  TestCompletionCallback callback;
  int rv = stream.Open(temp_file_path(), flags, callback.callback());
  EXPECT_THAT(callback.GetResult(rv), IsOk());

  int total_bytes_read = 0;

  std::string data_read;
  for (;;) {
    scoped_refptr<IOBufferWithSize> buf = new IOBufferWithSize(4);
    rv = stream.Read(buf.get(), buf->size(), callback.callback());
    rv = callback.GetResult(rv);
    EXPECT_LE(0, rv);
    if (rv <= 0)
      break;
    total_bytes_read += rv;
    data_read.append(buf->data(), rv);
  }
  EXPECT_EQ(file_size, total_bytes_read);
  EXPECT_EQ(kTestData, data_read);
}

TEST_F(FileStreamTest, Read_EarlyDelete) {
  int64_t file_size;
  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));

  std::unique_ptr<FileStream> stream(
      new FileStream(base::ThreadTaskRunnerHandle::Get()));
  int flags = base::File::FLAG_OPEN | base::File::FLAG_READ |
              base::File::FLAG_ASYNC;
  TestCompletionCallback callback;
  int rv = stream->Open(temp_file_path(), flags, callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
  EXPECT_THAT(callback.WaitForResult(), IsOk());

  scoped_refptr<IOBufferWithSize> buf = new IOBufferWithSize(4);
  rv = stream->Read(buf.get(), buf->size(), callback.callback());
  stream.reset();  // Delete instead of closing it.
  if (rv < 0) {
    EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
    // The callback should not be called if the request is cancelled.
    base::RunLoop().RunUntilIdle();
    EXPECT_FALSE(callback.have_result());
  } else {
    EXPECT_EQ(std::string(kTestData, rv), std::string(buf->data(), rv));
  }
}

TEST_F(FileStreamTest, Read_FromOffset) {
  int64_t file_size;
  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));

  FileStream stream(base::ThreadTaskRunnerHandle::Get());
  int flags = base::File::FLAG_OPEN | base::File::FLAG_READ |
              base::File::FLAG_ASYNC;
  TestCompletionCallback callback;
  int rv = stream.Open(temp_file_path(), flags, callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
  EXPECT_THAT(callback.WaitForResult(), IsOk());

  TestInt64CompletionCallback callback64;
  const int64_t kOffset = 3;
  rv = stream.Seek(kOffset, callback64.callback());
  ASSERT_THAT(rv, IsError(ERR_IO_PENDING));
  int64_t new_offset = callback64.WaitForResult();
  EXPECT_EQ(kOffset, new_offset);

  int total_bytes_read = 0;

  std::string data_read;
  for (;;) {
    scoped_refptr<IOBufferWithSize> buf = new IOBufferWithSize(4);
    rv = stream.Read(buf.get(), buf->size(), callback.callback());
    if (rv == ERR_IO_PENDING)
      rv = callback.WaitForResult();
    EXPECT_LE(0, rv);
    if (rv <= 0)
      break;
    total_bytes_read += rv;
    data_read.append(buf->data(), rv);
  }
  EXPECT_EQ(file_size - kOffset, total_bytes_read);
  EXPECT_EQ(kTestData + kOffset, data_read);
}

TEST_F(FileStreamTest, Write) {
  FileStream stream(base::ThreadTaskRunnerHandle::Get());
  int flags = base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE |
              base::File::FLAG_ASYNC;
  TestCompletionCallback callback;
  int rv = stream.Open(temp_file_path(), flags, callback.callback());
  EXPECT_THAT(callback.GetResult(rv), IsOk());

  int64_t file_size;
  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));
  EXPECT_EQ(0, file_size);

  scoped_refptr<IOBuffer> buf = CreateTestDataBuffer();
  rv = stream.Write(buf.get(), kTestDataSize, callback.callback());
  rv = callback.GetResult(rv);
  EXPECT_EQ(kTestDataSize, rv);

  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));
  EXPECT_EQ(kTestDataSize, file_size);

  std::string data_read;
  EXPECT_TRUE(base::ReadFileToString(temp_file_path(), &data_read));
  EXPECT_EQ(kTestData, data_read);
}

TEST_F(FileStreamTest, Write_EarlyDelete) {
  std::unique_ptr<FileStream> stream(
      new FileStream(base::ThreadTaskRunnerHandle::Get()));
  int flags = base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE |
              base::File::FLAG_ASYNC;
  TestCompletionCallback callback;
  int rv = stream->Open(temp_file_path(), flags, callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
  EXPECT_THAT(callback.WaitForResult(), IsOk());

  int64_t file_size;
  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));
  EXPECT_EQ(0, file_size);

  scoped_refptr<IOBufferWithSize> buf = CreateTestDataBuffer();
  rv = stream->Write(buf.get(), buf->size(), callback.callback());
  stream.reset();
  if (rv < 0) {
    EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
    // The callback should not be called if the request is cancelled.
    base::RunLoop().RunUntilIdle();
    EXPECT_FALSE(callback.have_result());
  } else {
    EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));
    EXPECT_EQ(file_size, rv);
  }
}

TEST_F(FileStreamTest, Write_FromOffset) {
  int64_t file_size;
  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));

  FileStream stream(base::ThreadTaskRunnerHandle::Get());
  int flags = base::File::FLAG_OPEN | base::File::FLAG_WRITE |
              base::File::FLAG_ASYNC;
  TestCompletionCallback callback;
  int rv = stream.Open(temp_file_path(), flags, callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
  EXPECT_THAT(callback.WaitForResult(), IsOk());

  TestInt64CompletionCallback callback64;
  const int64_t kOffset = kTestDataSize;
  rv = stream.Seek(kOffset, callback64.callback());
  ASSERT_THAT(rv, IsError(ERR_IO_PENDING));
  int64_t new_offset = callback64.WaitForResult();
  EXPECT_EQ(kTestDataSize, new_offset);

  int total_bytes_written = 0;

  scoped_refptr<IOBufferWithSize> buf = CreateTestDataBuffer();
  scoped_refptr<DrainableIOBuffer> drainable =
      new DrainableIOBuffer(buf.get(), buf->size());
  while (total_bytes_written != kTestDataSize) {
    rv = stream.Write(drainable.get(), drainable->BytesRemaining(),
                      callback.callback());
    if (rv == ERR_IO_PENDING)
      rv = callback.WaitForResult();
    EXPECT_LT(0, rv);
    if (rv <= 0)
      break;
    drainable->DidConsume(rv);
    total_bytes_written += rv;
  }
  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));
  EXPECT_EQ(file_size, kTestDataSize * 2);
}

TEST_F(FileStreamTest, BasicReadWrite) {
  int64_t file_size;
  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));

  std::unique_ptr<FileStream> stream(
      new FileStream(base::ThreadTaskRunnerHandle::Get()));
  int flags = base::File::FLAG_OPEN | base::File::FLAG_READ |
              base::File::FLAG_WRITE | base::File::FLAG_ASYNC;
  TestCompletionCallback callback;
  int rv = stream->Open(temp_file_path(), flags, callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
  EXPECT_THAT(callback.WaitForResult(), IsOk());

  int64_t total_bytes_read = 0;

  std::string data_read;
  for (;;) {
    scoped_refptr<IOBufferWithSize> buf = new IOBufferWithSize(4);
    rv = stream->Read(buf.get(), buf->size(), callback.callback());
    if (rv == ERR_IO_PENDING)
      rv = callback.WaitForResult();
    EXPECT_LE(0, rv);
    if (rv <= 0)
      break;
    total_bytes_read += rv;
    data_read.append(buf->data(), rv);
  }
  EXPECT_EQ(file_size, total_bytes_read);
  EXPECT_TRUE(data_read == kTestData);

  int total_bytes_written = 0;

  scoped_refptr<IOBufferWithSize> buf = CreateTestDataBuffer();
  scoped_refptr<DrainableIOBuffer> drainable =
      new DrainableIOBuffer(buf.get(), buf->size());
  while (total_bytes_written != kTestDataSize) {
    rv = stream->Write(drainable.get(), drainable->BytesRemaining(),
                       callback.callback());
    if (rv == ERR_IO_PENDING)
      rv = callback.WaitForResult();
    EXPECT_LT(0, rv);
    if (rv <= 0)
      break;
    drainable->DidConsume(rv);
    total_bytes_written += rv;
  }

  stream.reset();

  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));
  EXPECT_EQ(kTestDataSize * 2, file_size);
}

TEST_F(FileStreamTest, BasicWriteRead) {
  int64_t file_size;
  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));

  std::unique_ptr<FileStream> stream(
      new FileStream(base::ThreadTaskRunnerHandle::Get()));
  int flags = base::File::FLAG_OPEN | base::File::FLAG_READ |
              base::File::FLAG_WRITE | base::File::FLAG_ASYNC;
  TestCompletionCallback callback;
  int rv = stream->Open(temp_file_path(), flags, callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
  EXPECT_THAT(callback.WaitForResult(), IsOk());

  TestInt64CompletionCallback callback64;
  rv = stream->Seek(file_size, callback64.callback());
  ASSERT_THAT(rv, IsError(ERR_IO_PENDING));
  int64_t offset = callback64.WaitForResult();
  EXPECT_EQ(offset, file_size);

  int total_bytes_written = 0;

  scoped_refptr<IOBufferWithSize> buf = CreateTestDataBuffer();
  scoped_refptr<DrainableIOBuffer> drainable =
      new DrainableIOBuffer(buf.get(), buf->size());
  while (total_bytes_written != kTestDataSize) {
    rv = stream->Write(drainable.get(), drainable->BytesRemaining(),
                       callback.callback());
    if (rv == ERR_IO_PENDING)
      rv = callback.WaitForResult();
    EXPECT_LT(0, rv);
    if (rv <= 0)
      break;
    drainable->DidConsume(rv);
    total_bytes_written += rv;
  }

  EXPECT_EQ(kTestDataSize, total_bytes_written);

  rv = stream->Seek(0, callback64.callback());
  ASSERT_THAT(rv, IsError(ERR_IO_PENDING));
  offset = callback64.WaitForResult();
  EXPECT_EQ(0, offset);

  int total_bytes_read = 0;

  std::string data_read;
  for (;;) {
    scoped_refptr<IOBufferWithSize> buf = new IOBufferWithSize(4);
    rv = stream->Read(buf.get(), buf->size(), callback.callback());
    if (rv == ERR_IO_PENDING)
      rv = callback.WaitForResult();
    EXPECT_LE(0, rv);
    if (rv <= 0)
      break;
    total_bytes_read += rv;
    data_read.append(buf->data(), rv);
  }
  stream.reset();

  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));
  EXPECT_EQ(kTestDataSize * 2, file_size);

  EXPECT_EQ(kTestDataSize * 2, total_bytes_read);
  const std::string kExpectedFileData =
      std::string(kTestData) + std::string(kTestData);
  EXPECT_EQ(kExpectedFileData, data_read);
}

class TestWriteReadCompletionCallback {
 public:
  TestWriteReadCompletionCallback(FileStream* stream,
                                  int* total_bytes_written,
                                  int* total_bytes_read,
                                  std::string* data_read)
      : result_(0),
        have_result_(false),
        waiting_for_result_(false),
        stream_(stream),
        total_bytes_written_(total_bytes_written),
        total_bytes_read_(total_bytes_read),
        data_read_(data_read),
        callback_(base::Bind(&TestWriteReadCompletionCallback::OnComplete,
                             base::Unretained(this))),
        test_data_(CreateTestDataBuffer()),
        drainable_(new DrainableIOBuffer(test_data_.get(), kTestDataSize)) {}

  int WaitForResult() {
    DCHECK(!waiting_for_result_);
    while (!have_result_) {
      waiting_for_result_ = true;
      base::RunLoop().Run();
      waiting_for_result_ = false;
    }
    have_result_ = false;  // auto-reset for next callback
    return result_;
  }

  const CompletionCallback& callback() const { return callback_; }

  void ValidateWrittenData() {
    TestCompletionCallback callback;
    int rv = 0;
    for (;;) {
      scoped_refptr<IOBufferWithSize> buf = new IOBufferWithSize(4);
      rv = stream_->Read(buf.get(), buf->size(), callback.callback());
      if (rv == ERR_IO_PENDING) {
        base::MessageLoopCurrent::ScopedNestableTaskAllower allow;
        rv = callback.WaitForResult();
      }
      EXPECT_LE(0, rv);
      if (rv <= 0)
        break;
      *total_bytes_read_ += rv;
      data_read_->append(buf->data(), rv);
    }
  }

 private:
  void OnComplete(int result) {
    DCHECK_LT(0, result);
    *total_bytes_written_ += result;

    int rv;

    if (*total_bytes_written_ != kTestDataSize) {
      // Recurse to finish writing all data.
      int total_bytes_written = 0, total_bytes_read = 0;
      std::string data_read;
      TestWriteReadCompletionCallback callback(
          stream_, &total_bytes_written, &total_bytes_read, &data_read);
      rv = stream_->Write(
          drainable_.get(), drainable_->BytesRemaining(), callback.callback());
      DCHECK_EQ(ERR_IO_PENDING, rv);
      rv = callback.WaitForResult();
      drainable_->DidConsume(total_bytes_written);
      *total_bytes_written_ += total_bytes_written;
      *total_bytes_read_ += total_bytes_read;
      *data_read_ += data_read;
    } else {  // We're done writing all data.  Start reading the data.
      TestInt64CompletionCallback callback64;
      EXPECT_THAT(stream_->Seek(0, callback64.callback()),
                  IsError(ERR_IO_PENDING));
      {
        base::MessageLoopCurrent::ScopedNestableTaskAllower allow;
        EXPECT_LE(0, callback64.WaitForResult());
      }
    }

    result_ = *total_bytes_written_;
    have_result_ = true;
    if (waiting_for_result_)
      base::RunLoop::QuitCurrentWhenIdleDeprecated();
  }

  int result_;
  bool have_result_;
  bool waiting_for_result_;
  FileStream* stream_;
  int* total_bytes_written_;
  int* total_bytes_read_;
  std::string* data_read_;
  const CompletionCallback callback_;
  scoped_refptr<IOBufferWithSize> test_data_;
  scoped_refptr<DrainableIOBuffer> drainable_;

  DISALLOW_COPY_AND_ASSIGN(TestWriteReadCompletionCallback);
};

TEST_F(FileStreamTest, WriteRead) {
  int64_t file_size;
  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));

  std::unique_ptr<FileStream> stream(
      new FileStream(base::ThreadTaskRunnerHandle::Get()));
  int flags = base::File::FLAG_OPEN | base::File::FLAG_READ |
              base::File::FLAG_WRITE | base::File::FLAG_ASYNC;
  TestCompletionCallback open_callback;
  int rv = stream->Open(temp_file_path(), flags, open_callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
  EXPECT_THAT(open_callback.WaitForResult(), IsOk());

  TestInt64CompletionCallback callback64;
  EXPECT_THAT(stream->Seek(file_size, callback64.callback()),
              IsError(ERR_IO_PENDING));
  EXPECT_EQ(file_size, callback64.WaitForResult());

  int total_bytes_written = 0;
  int total_bytes_read = 0;
  std::string data_read;
  TestWriteReadCompletionCallback callback(stream.get(), &total_bytes_written,
                                           &total_bytes_read, &data_read);

  scoped_refptr<IOBufferWithSize> buf = CreateTestDataBuffer();
  rv = stream->Write(buf.get(), buf->size(), callback.callback());
  if (rv == ERR_IO_PENDING)
    rv = callback.WaitForResult();
  EXPECT_LT(0, rv);
  EXPECT_EQ(kTestDataSize, total_bytes_written);

  callback.ValidateWrittenData();

  stream.reset();

  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));
  EXPECT_EQ(kTestDataSize * 2, file_size);

  EXPECT_EQ(kTestDataSize * 2, total_bytes_read);
  const std::string kExpectedFileData =
      std::string(kTestData) + std::string(kTestData);
  EXPECT_EQ(kExpectedFileData, data_read);
}

class TestWriteCloseCompletionCallback {
 public:
  TestWriteCloseCompletionCallback(FileStream* stream, int* total_bytes_written)
      : result_(0),
        have_result_(false),
        waiting_for_result_(false),
        stream_(stream),
        total_bytes_written_(total_bytes_written),
        callback_(base::Bind(&TestWriteCloseCompletionCallback::OnComplete,
                             base::Unretained(this))),
        test_data_(CreateTestDataBuffer()),
        drainable_(new DrainableIOBuffer(test_data_.get(), kTestDataSize)) {}

  int WaitForResult() {
    DCHECK(!waiting_for_result_);
    while (!have_result_) {
      waiting_for_result_ = true;
      base::RunLoop().Run();
      waiting_for_result_ = false;
    }
    have_result_ = false;  // auto-reset for next callback
    return result_;
  }

  const CompletionCallback& callback() const { return callback_; }

 private:
  void OnComplete(int result) {
    DCHECK_LT(0, result);
    *total_bytes_written_ += result;

    int rv;

    if (*total_bytes_written_ != kTestDataSize) {
      // Recurse to finish writing all data.
      int total_bytes_written = 0;
      TestWriteCloseCompletionCallback callback(stream_, &total_bytes_written);
      rv = stream_->Write(
          drainable_.get(), drainable_->BytesRemaining(), callback.callback());
      DCHECK_EQ(ERR_IO_PENDING, rv);
      rv = callback.WaitForResult();
      drainable_->DidConsume(total_bytes_written);
      *total_bytes_written_ += total_bytes_written;
    }

    result_ = *total_bytes_written_;
    have_result_ = true;
    if (waiting_for_result_)
      base::RunLoop::QuitCurrentWhenIdleDeprecated();
  }

  int result_;
  bool have_result_;
  bool waiting_for_result_;
  FileStream* stream_;
  int* total_bytes_written_;
  const CompletionCallback callback_;
  scoped_refptr<IOBufferWithSize> test_data_;
  scoped_refptr<DrainableIOBuffer> drainable_;

  DISALLOW_COPY_AND_ASSIGN(TestWriteCloseCompletionCallback);
};

TEST_F(FileStreamTest, WriteClose) {
  int64_t file_size;
  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));

  std::unique_ptr<FileStream> stream(
      new FileStream(base::ThreadTaskRunnerHandle::Get()));
  int flags = base::File::FLAG_OPEN | base::File::FLAG_READ |
              base::File::FLAG_WRITE | base::File::FLAG_ASYNC;
  TestCompletionCallback open_callback;
  int rv = stream->Open(temp_file_path(), flags, open_callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
  EXPECT_THAT(open_callback.WaitForResult(), IsOk());

  TestInt64CompletionCallback callback64;
  EXPECT_THAT(stream->Seek(file_size, callback64.callback()),
              IsError(ERR_IO_PENDING));
  EXPECT_EQ(file_size, callback64.WaitForResult());

  int total_bytes_written = 0;
  TestWriteCloseCompletionCallback callback(stream.get(), &total_bytes_written);

  scoped_refptr<IOBufferWithSize> buf = CreateTestDataBuffer();
  rv = stream->Write(buf.get(), buf->size(), callback.callback());
  if (rv == ERR_IO_PENDING)
    total_bytes_written = callback.WaitForResult();
  EXPECT_LT(0, total_bytes_written);
  EXPECT_EQ(kTestDataSize, total_bytes_written);

  stream.reset();

  EXPECT_TRUE(base::GetFileSize(temp_file_path(), &file_size));
  EXPECT_EQ(kTestDataSize * 2, file_size);
}

TEST_F(FileStreamTest, OpenAndDelete) {
  base::Thread worker_thread("StreamTest");
  ASSERT_TRUE(worker_thread.Start());

  bool prev = base::ThreadRestrictions::SetIOAllowed(false);
  std::unique_ptr<FileStream> stream(
      new FileStream(worker_thread.task_runner()));
  int flags = base::File::FLAG_OPEN | base::File::FLAG_WRITE |
              base::File::FLAG_ASYNC;
  TestCompletionCallback open_callback;
  int rv = stream->Open(temp_file_path(), flags, open_callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));

  // Delete the stream without waiting for the open operation to be
  // complete. Should be safe.
  stream.reset();

  // Force an operation through the worker.
  std::unique_ptr<FileStream> stream2(
      new FileStream(worker_thread.task_runner()));
  TestCompletionCallback open_callback2;
  rv = stream2->Open(temp_file_path(), flags, open_callback2.callback());
  EXPECT_THAT(open_callback2.GetResult(rv), IsOk());
  stream2.reset();

  // open_callback won't be called.
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(open_callback.have_result());
  base::ThreadRestrictions::SetIOAllowed(prev);
}

// Verify that Write() errors are mapped correctly.
TEST_F(FileStreamTest, WriteError) {
  // Try opening file as read-only and then writing to it using FileStream.
  uint32_t flags =
      base::File::FLAG_OPEN | base::File::FLAG_READ | base::File::FLAG_ASYNC;

  base::File file(temp_file_path(), flags);
  ASSERT_TRUE(file.IsValid());

  std::unique_ptr<FileStream> stream(
      new FileStream(std::move(file), base::ThreadTaskRunnerHandle::Get()));

  scoped_refptr<IOBuffer> buf = new IOBuffer(1);
  buf->data()[0] = 0;

  TestCompletionCallback callback;
  int rv = stream->Write(buf.get(), 1, callback.callback());
  if (rv == ERR_IO_PENDING)
    rv = callback.WaitForResult();
  EXPECT_LT(rv, 0);

  stream.reset();
  base::RunLoop().RunUntilIdle();
}

// Verify that Read() errors are mapped correctly.
TEST_F(FileStreamTest, ReadError) {
  // Try opening file for write and then reading from it using FileStream.
  uint32_t flags =
      base::File::FLAG_OPEN | base::File::FLAG_WRITE | base::File::FLAG_ASYNC;

  base::File file(temp_file_path(), flags);
  ASSERT_TRUE(file.IsValid());

  std::unique_ptr<FileStream> stream(
      new FileStream(std::move(file), base::ThreadTaskRunnerHandle::Get()));

  scoped_refptr<IOBuffer> buf = new IOBuffer(1);
  TestCompletionCallback callback;
  int rv = stream->Read(buf.get(), 1, callback.callback());
  if (rv == ERR_IO_PENDING)
    rv = callback.WaitForResult();
  EXPECT_LT(rv, 0);

  stream.reset();
  base::RunLoop().RunUntilIdle();
}

#if defined(OS_ANDROID)
TEST_F(FileStreamTest, ContentUriRead) {
  base::FilePath test_dir;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &test_dir);
  test_dir = test_dir.AppendASCII("net");
  test_dir = test_dir.AppendASCII("data");
  test_dir = test_dir.AppendASCII("file_stream_unittest");
  ASSERT_TRUE(base::PathExists(test_dir));
  base::FilePath image_file = test_dir.Append(FILE_PATH_LITERAL("red.png"));

  // Insert the image into MediaStore. MediaStore will do some conversions, and
  // return the content URI.
  base::FilePath path = base::InsertImageIntoMediaStore(image_file);
  EXPECT_TRUE(path.IsContentUri());
  EXPECT_TRUE(base::PathExists(path));
  int64_t file_size;
  EXPECT_TRUE(base::GetFileSize(path, &file_size));
  EXPECT_LT(0, file_size);

  FileStream stream(base::ThreadTaskRunnerHandle::Get());
  int flags = base::File::FLAG_OPEN | base::File::FLAG_READ |
              base::File::FLAG_ASYNC;
  TestCompletionCallback callback;
  int rv = stream.Open(path, flags, callback.callback());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));
  EXPECT_THAT(callback.WaitForResult(), IsOk());

  int total_bytes_read = 0;

  std::string data_read;
  for (;;) {
    scoped_refptr<IOBufferWithSize> buf = new IOBufferWithSize(4);
    rv = stream.Read(buf.get(), buf->size(), callback.callback());
    if (rv == ERR_IO_PENDING)
      rv = callback.WaitForResult();
    EXPECT_LE(0, rv);
    if (rv <= 0)
      break;
    total_bytes_read += rv;
    data_read.append(buf->data(), rv);
  }
  EXPECT_EQ(file_size, total_bytes_read);
}
#endif

}  // namespace

}  // namespace net
