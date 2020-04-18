// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/redirect_to_file_resource_handler.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/strings/string_piece.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/loader/mock_resource_loader.h"
#include "content/browser/loader/temporary_file_stream.h"
#include "content/browser/loader/test_resource_handler.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/completion_callback.h"
#include "net/base/completion_once_callback.h"
#include "net/base/file_stream.h"
#include "net/base/io_buffer.h"
#include "net/base/mime_sniffer.h"
#include "net/base/net_errors.h"
#include "net/base/request_priority.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_test_util.h"
#include "services/network/public/cpp/resource_response.h"
#include "storage/browser/blob/shareable_file_reference.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {
namespace {

// The maximum size for which the initial read will always be sync, even when
// the wrote completes asynchronously.  See
// RedirectToFileResourceHandler::BufIsFull().
const int kMaxInitialSyncReadSize =
    RedirectToFileResourceHandler::kInitialReadBufSize -
    2 * net::kMaxBytesToSniff - 1;

// Used to indicate whether FileStream operations and the lower-layer
// TestResourceHandler operations should complete immediately or by
// asynchronously invoking a callback. Each test is run with all operations set
// by default to each mode, though some tests override the mode of some
// operations.
enum class CompletionMode {
  SYNC,
  ASYNC,
};

// Mock in-memory net::FileStream implementation that can be configured to
// return errors and complete operations synchronously or asynchronously.
class MockFileStream : public net::FileStream {
 public:
  struct OperationResult {
    OperationResult(int result, CompletionMode completion_mode)
        : result(result), completion_mode(completion_mode) {}

    OperationResult()
        : OperationResult(net::ERR_UNEXPECTED, CompletionMode::SYNC) {}

    int result;
    CompletionMode completion_mode;
  };

  MockFileStream() : FileStream(base::ThreadTaskRunnerHandle::Get()) {}

  ~MockFileStream() override {
    EXPECT_EQ(expect_closed_, closed_);
    // Most of these tests write 32k or more, which is a bit much for the
    // command line.
    EXPECT_TRUE(expected_written_data_ == written_data_);
  }

  // net::FileStream implementation:

  int Open(const base::FilePath& path,
           int open_flags,
           net::CompletionOnceCallback callback) override {
    return ReturnResult(open_result_, std::move(callback));
  }

  int Close(net::CompletionOnceCallback callback) override {
    EXPECT_FALSE(closed_);
    int result = ReturnResult(
        close_result_,
        base::BindOnce(&MockFileStream::SetClosedAndRunCallback,
                       base::Unretained(this), std::move(callback)));
    if (result != net::ERR_IO_PENDING)
      closed_ = true;
    return result;
  }

  bool IsOpen() const override {
    NOTREACHED();
    return false;
  }

  int Seek(int64_t offset, net::Int64CompletionOnceCallback callback) override {
    NOTREACHED();
    return net::ERR_UNEXPECTED;
  }

  int Read(net::IOBuffer* buf,
           int buf_len,
           net::CompletionOnceCallback callback) override {
    NOTREACHED();
    return net::ERR_UNEXPECTED;
  }

  int Write(net::IOBuffer* buf,
            int buf_len,
            net::CompletionOnceCallback callback) override {
    // 0-byte writes aren't allowed.
    EXPECT_GT(buf_len, 0);

    OperationResult write_result = next_write_result_;
    next_write_result_ = all_write_results_;
    if (write_result.result > buf_len)
      write_result.result = buf_len;
    if (write_result.result > 0)
      written_data_ += std::string(buf->data(), write_result.result);

    return ReturnResult(write_result, std::move(callback));
  }

  int Flush(net::CompletionOnceCallback callback) override {
    NOTREACHED();
    return net::ERR_UNEXPECTED;
  }

  void set_open_result(OperationResult open_result) {
    open_result_ = open_result;
  }
  void set_close_result(OperationResult close_result) {
    close_result_ = close_result;
  }

  // Sets the result for all write operations. Returned result is capped at
  // number of bytes the consumer actually tried to write. Overrides
  // |next_write_result_|.
  void set_all_write_results(OperationResult all_write_results) {
    next_write_result_ = all_write_results_ = all_write_results;
  }

  // Sets the result of only the next write operation.
  void set_next_write_result(OperationResult next_write_result) {
    next_write_result_ = next_write_result;
  }

  void set_expected_written_data(const std::string& expected_written_data) {
    expected_written_data_ = expected_written_data;
  }

  // Sets whether the file should expect to be closed.
  void set_expect_closed(bool expect_closed) { expect_closed_ = expect_closed; }

 private:
  void SetClosedAndRunCallback(net::CompletionOnceCallback callback,
                               int result) {
    EXPECT_FALSE(closed_);
    closed_ = true;
    std::move(callback).Run(result);
  }

  int ReturnResult(OperationResult result,
                   net::CompletionOnceCallback callback) {
    if (result.completion_mode == CompletionMode::SYNC)
      return result.result;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), result.result));
    return net::ERR_IO_PENDING;
  }

  OperationResult open_result_;
  OperationResult close_result_;
  OperationResult next_write_result_;
  OperationResult all_write_results_;

  std::string expected_written_data_;
  std::string written_data_;

  bool expect_closed_ = false;
  bool closed_ = false;

  DISALLOW_COPY_AND_ASSIGN(MockFileStream);
};

class RedirectToFileResourceHandlerTest
    : public testing::TestWithParam<CompletionMode> {
 public:
  RedirectToFileResourceHandlerTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP),
        url_request_(
            url_request_context_.CreateRequest(GURL("foo://bar/"),
                                               net::DEFAULT_PRIORITY,
                                               &url_request_delegate_,
                                               TRAFFIC_ANNOTATION_FOR_TESTS)) {
    base::CreateTemporaryFile(&temp_file_path_);
    std::unique_ptr<TestResourceHandler> test_handler =
        std::make_unique<TestResourceHandler>();
    test_handler->set_expect_on_data_downloaded(true);
    if (GetParam() == CompletionMode::ASYNC) {
      // Don't defer OnResponseCompleted, by default, since that's really
      // unusual.
      test_handler->set_defer_on_response_started(true);
      test_handler->set_defer_on_will_start(true);
    }
    test_handler_ = test_handler->GetWeakPtr();

    redirect_to_file_handler_ = std::make_unique<RedirectToFileResourceHandler>(
        std::move(test_handler), url_request_.get());
    mock_loader_ =
        std::make_unique<MockResourceLoader>(redirect_to_file_handler_.get());
    redirect_to_file_handler_->SetCreateTemporaryFileStreamFunctionForTesting(
        base::Bind(&RedirectToFileResourceHandlerTest::
                       SetCreateTemporaryFileStreamCallback,
                   base::Unretained(this)));

    file_stream_ = std::make_unique<MockFileStream>();
    file_stream_->set_open_result(
        MockFileStream::OperationResult(net::OK, GetParam()));
    file_stream_->set_all_write_results(MockFileStream::OperationResult(
        std::numeric_limits<int>::max(), GetParam()));
    file_stream_->set_close_result(
        MockFileStream::OperationResult(net::OK, GetParam()));
  }

  ~RedirectToFileResourceHandlerTest() override {
    EXPECT_FALSE(test_handler_->on_read_completed_called());

    // This should post a task to delete the temporary file.
    redirect_to_file_handler_.reset();
    mock_loader_.reset();
    url_request_.reset();
    // This should delete the temporary file, and ensure
    // MockFileStream::Cancel() is called.
    base::RunLoop().RunUntilIdle();

    EXPECT_FALSE(base::PathExists(temp_file_path_));
  }

  // Creates a test string of the specified length, and sets that as the
  // expected data written to |file_stream_|.
  std::string CreateTestData(size_t length) {
    std::string test_data;
    test_data.reserve(length);
    for (size_t i = 0; i < length; ++i)
      test_data.push_back(static_cast<char>(i % 256));
    file_stream_->set_expected_written_data(test_data);
    return test_data;
  }

  // The "CreateTemporaryFileStream" method invoked by the
  // RedirectToFileResourceHandler.  Just sets a callback that will be invoked
  // directly.
  void SetCreateTemporaryFileStreamCallback(
      const CreateTemporaryFileStreamCallback& create_file_stream_callback) {
    create_file_stream_callback_ = create_file_stream_callback;
  }

  void PerformOnWillStart() {
    MockResourceLoader::Status expected_status;
    if (GetParam() == CompletionMode::ASYNC) {
      expected_status = MockResourceLoader::Status::CALLBACK_PENDING;
    } else {
      expected_status = MockResourceLoader::Status::IDLE;
    }
    EXPECT_EQ(expected_status, mock_loader_->OnWillStart(url_request_->url()));
  }

  // Sets up the file stream or error, and performs the file callback.
  void PerformCreateFile(base::File::Error file_error) {
    DCHECK(file_stream_);

    file_stream_->set_expect_closed(file_error == base::File::FILE_OK);
    if (file_error != base::File::FILE_OK)
      file_stream_ = nullptr;

    base::ResetAndReturn(&create_file_stream_callback_)
        .Run(file_error, std::move(file_stream_),
             // Not really used by the test, but the ResourceHandler expects it
             // to be non-null.
             storage::ShareableFileReference::GetOrCreate(
                 temp_file_path_,
                 storage::ShareableFileReference::DELETE_ON_FINAL_RELEASE,
                 base::ThreadTaskRunnerHandle::Get().get())
                 .get());
  }

  // Simulates starting the request, the response starting, and stream creation
  // completing with the specified error code. Has |test_handler_| resume the
  // request, if needed. Returns final status of |mock_loader_|.
  MockResourceLoader::Status StartAndCreateStream(base::File::Error file_error)
      WARN_UNUSED_RESULT {
    PerformOnWillStart();

    // Create the file right away.
    PerformCreateFile(file_error);

    // If this is an async test, |test_handler_| will defer the OnWillStart
    // event on success (On error, its OnWillStart method is not called).
    if (file_error == base::File::FILE_OK &&
        GetParam() == CompletionMode::ASYNC) {
      EXPECT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
                mock_loader_->status());
      test_handler_->Resume();
      mock_loader_->WaitUntilIdleOrCanceled();
    }
    EXPECT_NE(MockResourceLoader::Status::CALLBACK_PENDING,
              mock_loader_->status());
    return mock_loader_->status();
  }

  // Convenience wrapper for MockLoader methods that will Resume |test_handler_|
  // and wait for it to resume the request if running an async test.
  MockResourceLoader::Status OnResponseStartedAndWaitForResult()
      WARN_UNUSED_RESULT {
    mock_loader_->OnResponseStarted(
        base::MakeRefCounted<network::ResourceResponse>());
    if (GetParam() == CompletionMode::ASYNC) {
      EXPECT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
                mock_loader_->status());
      test_handler_->Resume();
      mock_loader_->WaitUntilIdleOrCanceled();
    }
    EXPECT_NE(MockResourceLoader::Status::CALLBACK_PENDING,
              mock_loader_->status());
    return mock_loader_->status();
  }

  // Utility method that simulates a final 0-byte read and response completed
  // events, and checks that completion is handled correctly. Expects all data
  // to already have been written to the file.
  void CompleteRequestSuccessfully(int expected_total_bytes_downloaded) {
    // The loader should be idle and all the data should have already been
    // processed.
    ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());
    EXPECT_EQ(expected_total_bytes_downloaded,
              test_handler_->total_bytes_downloaded());

    ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
    ASSERT_EQ(MockResourceLoader::Status::IDLE,
              mock_loader_->OnReadCompleted(""));
    ASSERT_EQ(MockResourceLoader::Status::IDLE,
              mock_loader_->OnResponseCompleted(
                  net::URLRequestStatus::FromError(net::OK)));
    EXPECT_EQ(expected_total_bytes_downloaded,
              test_handler_->total_bytes_downloaded());
    EXPECT_EQ(net::URLRequestStatus::SUCCESS,
              test_handler_->final_status().status());
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;
  base::FilePath temp_file_path_;
  net::TestURLRequestContext url_request_context_;
  net::TestDelegate url_request_delegate_;
  base::WeakPtr<TestResourceHandler> test_handler_;
  std::unique_ptr<net::URLRequest> url_request_;
  std::unique_ptr<MockResourceLoader> mock_loader_;
  std::unique_ptr<RedirectToFileResourceHandler> redirect_to_file_handler_;
  std::unique_ptr<MockFileStream> file_stream_;

  CreateTemporaryFileStreamCallback create_file_stream_callback_;
};

TEST_P(RedirectToFileResourceHandlerTest, EmptyBody) {
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  CompleteRequestSuccessfully(0);
}

TEST_P(RedirectToFileResourceHandlerTest, SingleBodyRead) {
  std::string test_data = CreateTestData(kMaxInitialSyncReadSize);

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(test_data));
  // Wait for the write to complete, in the async case.
  base::RunLoop().RunUntilIdle();

  CompleteRequestSuccessfully(test_data.size());
}

TEST_P(RedirectToFileResourceHandlerTest, SingleBodyReadDelayedFileOnResponse) {
  std::string test_data = CreateTestData(kMaxInitialSyncReadSize);

  PerformOnWillStart();
  if (GetParam() == CompletionMode::ASYNC) {
    EXPECT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
              mock_loader_->status());
    test_handler_->Resume();
    mock_loader_->WaitUntilIdleOrCanceled();
  }
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());
  mock_loader_->OnResponseStarted(
      base::MakeRefCounted<network::ResourceResponse>());
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->status());

  PerformCreateFile(base::File::FILE_OK);

  if (GetParam() == CompletionMode::ASYNC) {
    EXPECT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
              mock_loader_->status());
    test_handler_->Resume();
    mock_loader_->WaitUntilIdleOrCanceled();
  }
  EXPECT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(test_data));
  // Wait for the write to complete, in the async case.
  base::RunLoop().RunUntilIdle();

  CompleteRequestSuccessfully(test_data.size());
}

TEST_P(RedirectToFileResourceHandlerTest, SingleBodyReadDelayedFileError) {
  std::string test_data = CreateTestData(0);

  PerformOnWillStart();
  if (GetParam() == CompletionMode::ASYNC) {
    EXPECT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
              mock_loader_->status());
    test_handler_->Resume();
    mock_loader_->WaitUntilIdleOrCanceled();
  }
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());
  mock_loader_->OnResponseStarted(
      base::MakeRefCounted<network::ResourceResponse>());
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->status());

  PerformCreateFile(base::File::FILE_ERROR_FAILED);

  EXPECT_EQ(MockResourceLoader::Status::CANCELED, mock_loader_->status());
  EXPECT_EQ(0, test_handler_->on_response_completed_called());
  EXPECT_EQ(net::ERR_FAILED, mock_loader_->error_code());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseCompleted(
                net::URLRequestStatus::FromError(net::ERR_FAILED)));
  EXPECT_FALSE(test_handler_->final_status().is_success());
  EXPECT_EQ(net::ERR_FAILED, test_handler_->final_status().error());
}

TEST_P(RedirectToFileResourceHandlerTest, ManySequentialBodyReads) {
  const size_t kBytesPerRead = 128;
  std::string test_data =
      CreateTestData(RedirectToFileResourceHandler::kInitialReadBufSize);

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  for (size_t offset = 0; offset < test_data.length();
       offset += kBytesPerRead) {
    ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
    size_t length = std::min(kBytesPerRead, test_data.length() - offset);
    ASSERT_EQ(MockResourceLoader::Status::IDLE,
              mock_loader_->OnReadCompleted(
                  base::StringPiece(test_data.data() + offset, length)));
    // Spin the message loop, to allow async writes to complete.
    base::RunLoop().RunUntilIdle();
  }

  CompleteRequestSuccessfully(test_data.size());
}

TEST_P(RedirectToFileResourceHandlerTest, PartialWrites) {
  std::string test_data = CreateTestData(kMaxInitialSyncReadSize);
  file_stream_->set_all_write_results(MockFileStream::OperationResult(
      RedirectToFileResourceHandler::kInitialReadBufSize / 50, GetParam()));

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(test_data));
  // Wait for the writes to complete, in the async case.
  base::RunLoop().RunUntilIdle();

  CompleteRequestSuccessfully(test_data.size());
}

// Same as above, but read enough data to defer reading the body.
TEST_P(RedirectToFileResourceHandlerTest, PartialWrites2) {
  std::string test_data =
      CreateTestData(RedirectToFileResourceHandler::kInitialReadBufSize);
  // Async reads, as otherwise reading won't be defered.
  file_stream_->set_all_write_results(MockFileStream::OperationResult(
      RedirectToFileResourceHandler::kInitialReadBufSize / 50,
      CompletionMode::ASYNC));

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->OnReadCompleted(test_data));
  // Wait for the writes to complete.
  base::RunLoop().RunUntilIdle();

  CompleteRequestSuccessfully(test_data.size());
}

TEST_P(RedirectToFileResourceHandlerTest, ReceiveDataWhileWritingBody) {
  const int kFirstWriteSize = 100;

  // This test only makes sense when reads are async.
  file_stream_->set_all_write_results(MockFileStream::OperationResult(
      std::numeric_limits<int>::max(), CompletionMode::ASYNC));

  // Will use multiple writes, with a combined size such that they don't
  // saturate the buffer.
  std::string test_data = CreateTestData(kMaxInitialSyncReadSize);

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(
      MockResourceLoader::Status::IDLE,
      mock_loader_->OnReadCompleted(test_data.substr(0, kFirstWriteSize)));
  // Next read completes before first write succeeds.
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(test_data.substr(
                kFirstWriteSize, sizeof(test_data) - kFirstWriteSize)));
  EXPECT_EQ(0, test_handler_->total_bytes_downloaded());

  // Wait for both writes to succeed.
  base::RunLoop().RunUntilIdle();

  CompleteRequestSuccessfully(test_data.size());
}

TEST_P(RedirectToFileResourceHandlerTest, ReceiveDataAndDeferWhileWritingBody) {
  const int kFirstWriteSize = 100;

  // This test only makes sense when reads are async.
  file_stream_->set_all_write_results(MockFileStream::OperationResult(
      std::numeric_limits<int>::max(), CompletionMode::ASYNC));

  // Will use multiple writes, with a combined size such that they saturate the
  // buffer.
  std::string test_data =
      CreateTestData(RedirectToFileResourceHandler::kInitialReadBufSize);

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(
      MockResourceLoader::Status::IDLE,
      mock_loader_->OnReadCompleted(test_data.substr(0, kFirstWriteSize)));
  // Next read completes before first write succeeds.
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->OnReadCompleted(test_data.substr(
                kFirstWriteSize, sizeof(test_data) - kFirstWriteSize)));
  EXPECT_EQ(0, test_handler_->total_bytes_downloaded());

  // Wait for both writes to succeed.
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());

  CompleteRequestSuccessfully(test_data.size());
}

TEST_P(RedirectToFileResourceHandlerTest,
       ExpandBufferCapacityManySequentialBodyReads) {
  // The buffer is only resized when reads are async.
  file_stream_->set_all_write_results(MockFileStream::OperationResult(
      std::numeric_limits<int>::max(), CompletionMode::ASYNC));

  const int kInitialReadSize =
      RedirectToFileResourceHandler::kInitialReadBufSize;
  const int kMaxReadSize = RedirectToFileResourceHandler::kMaxReadBufSize;
  int next_read_size = kInitialReadSize;
  int total_read_bytes = 0;
  // Populate |read_sizes| with expected buffer sizes if each previous read
  // filled the entire buffer.
  std::vector<size_t> read_sizes;
  while (true) {
    total_read_bytes += next_read_size;
    read_sizes.push_back(next_read_size);
    if (next_read_size == kMaxReadSize)
      break;
    next_read_size = std::min(2 * next_read_size, kMaxReadSize);
  }
  // Once the max is reached, do another round to make sure it isn't increased.
  total_read_bytes += kMaxReadSize;
  read_sizes.push_back(kMaxReadSize);

  std::string test_data = CreateTestData(total_read_bytes);

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  int offset = 0;
  for (int read_size : read_sizes) {
    ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
    ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
              mock_loader_->OnReadCompleted(
                  base::StringPiece(test_data.data() + offset, read_size)));
    offset += read_size;

    EXPECT_EQ(read_size, redirect_to_file_handler_->GetBufferSizeForTesting());

    base::RunLoop().RunUntilIdle();
    ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());
  }

  CompleteRequestSuccessfully(test_data.size());
}

TEST_P(RedirectToFileResourceHandlerTest, CompletedWhileWritingBody) {
  // This test only makes sense when reads are async.
  file_stream_->set_all_write_results(MockFileStream::OperationResult(
      std::numeric_limits<int>::max(), CompletionMode::ASYNC));

  std::string test_data = CreateTestData(kMaxInitialSyncReadSize);

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(test_data));
  EXPECT_EQ(0, test_handler_->total_bytes_downloaded());

  // While data is being written to the disk, the request completes.
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(""));
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->OnResponseCompleted(
                net::URLRequestStatus::FromError(net::OK)));

  // Wait for the write to complete and the final status sent to the
  // TestHandler.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(static_cast<int>(test_data.size()),
            test_handler_->total_bytes_downloaded());
  EXPECT_EQ(net::URLRequestStatus::SUCCESS,
            test_handler_->final_status().status());
}

TEST_P(RedirectToFileResourceHandlerTest,
       CompletedWhileWritingBodyAndWritePending) {
  const int kFirstWriteSize = 100;

  // This test only makes sense when reads are async.
  file_stream_->set_all_write_results(MockFileStream::OperationResult(
      std::numeric_limits<int>::max(), CompletionMode::ASYNC));

  // Will use multiple writes, with a combined size such that they don't
  // saturate the buffer.
  std::string test_data = CreateTestData(kMaxInitialSyncReadSize);

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(
      MockResourceLoader::Status::IDLE,
      mock_loader_->OnReadCompleted(test_data.substr(0, kFirstWriteSize)));
  // Next read completes before first write succeeds.
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(test_data.substr(
                kFirstWriteSize, sizeof(test_data) - kFirstWriteSize)));
  EXPECT_EQ(0, test_handler_->total_bytes_downloaded());

  // While the first write is still going on, the request completes.
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(""));
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->OnResponseCompleted(
                net::URLRequestStatus::FromError(net::OK)));

  // Wait for both writes to complete and the final status to be sent to the
  // TestHandler.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(static_cast<int>(test_data.size()),
            test_handler_->total_bytes_downloaded());
  EXPECT_EQ(net::URLRequestStatus::SUCCESS,
            test_handler_->final_status().status());
}

TEST_P(RedirectToFileResourceHandlerTest, SingleBodyReadAndFail) {
  std::string test_data = CreateTestData(kMaxInitialSyncReadSize);

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(test_data));

  // Wait for the write to complete.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(static_cast<int>(test_data.size()),
            test_handler_->total_bytes_downloaded());

  // Next read fails and request is torn down synchronously.
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseCompleted(
                net::URLRequestStatus::FromError(net::ERR_FAILED)));

  EXPECT_FALSE(test_handler_->final_status().is_success());
  EXPECT_EQ(net::ERR_FAILED, test_handler_->final_status().error());
}

TEST_P(RedirectToFileResourceHandlerTest, FailedWhileWritingBody) {
  // This test only makes sense when reads are async.
  file_stream_->set_all_write_results(MockFileStream::OperationResult(
      std::numeric_limits<int>::max(), CompletionMode::ASYNC));

  std::string test_data = CreateTestData(kMaxInitialSyncReadSize);

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(test_data));
  EXPECT_EQ(0, test_handler_->total_bytes_downloaded());

  // While data is being written to the disk, the request fails.
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->OnResponseCompleted(
                net::URLRequestStatus::FromError(net::ERR_FAILED)));

  // Wait for the write to complete and the final status sent to the
  // TestHandler.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(static_cast<int>(test_data.size()),
            test_handler_->total_bytes_downloaded());
  EXPECT_FALSE(test_handler_->final_status().is_success());
  EXPECT_EQ(net::ERR_FAILED, test_handler_->final_status().error());
}

TEST_P(RedirectToFileResourceHandlerTest,
       FailededWhileWritingBodyAndWritePending) {
  const int kFirstWriteSize = 100;

  // This test only makes sense when reads are async.
  file_stream_->set_all_write_results(MockFileStream::OperationResult(
      std::numeric_limits<int>::max(), CompletionMode::ASYNC));

  // Will use multiple writes, with a combined size such that they don't
  // saturate the buffer.
  std::string test_data = CreateTestData(kMaxInitialSyncReadSize);

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(
      MockResourceLoader::Status::IDLE,
      mock_loader_->OnReadCompleted(test_data.substr(0, kFirstWriteSize)));
  // Next read completes before first write succeeds.
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(test_data.substr(
                kFirstWriteSize, sizeof(test_data) - kFirstWriteSize)));

  // While the first write is still going on, the request fails.
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->OnResponseCompleted(
                net::URLRequestStatus::FromError(net::ERR_FAILED)));
  EXPECT_EQ(0, test_handler_->total_bytes_downloaded());

  // Wait for both writes to complete and the final status to be sent to the
  // TestHandler.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(static_cast<int>(test_data.size()),
            test_handler_->total_bytes_downloaded());
  EXPECT_FALSE(test_handler_->final_status().is_success());
  EXPECT_EQ(net::ERR_FAILED, test_handler_->final_status().error());
}

TEST_P(RedirectToFileResourceHandlerTest, CreateFileFails) {
  ASSERT_EQ(MockResourceLoader::Status::CANCELED,
            StartAndCreateStream(base::File::FILE_ERROR_FAILED));

  EXPECT_EQ(0, test_handler_->on_response_completed_called());
  EXPECT_EQ(net::ERR_FAILED, mock_loader_->error_code());
  if (GetParam() == CompletionMode::ASYNC) {
    EXPECT_EQ(MockResourceLoader::Status::CANCELED, mock_loader_->status());
    ASSERT_EQ(MockResourceLoader::Status::IDLE,
              mock_loader_->OnResponseCompletedFromExternalOutOfBandCancel(
                  net::URLRequestStatus::FromError(net::ERR_FAILED)));
  } else {
    ASSERT_EQ(MockResourceLoader::Status::IDLE,
              mock_loader_->OnResponseCompleted(
                  net::URLRequestStatus::FromError(net::ERR_FAILED)));
  }

  EXPECT_EQ(0, test_handler_->total_bytes_downloaded());
  EXPECT_FALSE(test_handler_->final_status().is_success());
  EXPECT_EQ(net::ERR_FAILED, test_handler_->final_status().error());
}

TEST_P(RedirectToFileResourceHandlerTest, FirstWriteFails) {
  std::string test_data = CreateTestData(kMaxInitialSyncReadSize);
  file_stream_->set_expected_written_data("");
  file_stream_->set_next_write_result(
      MockFileStream::OperationResult(net::ERR_FAILED, GetParam()));

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  mock_loader_->OnReadCompleted(test_data);
  // Wait for the write to complete, in the async case.
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(MockResourceLoader::Status::CANCELED, mock_loader_->status());

  EXPECT_EQ(net::ERR_FAILED, mock_loader_->error_code());
  EXPECT_EQ(0, test_handler_->on_response_completed_called());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseCompleted(
                net::URLRequestStatus::FromError(net::ERR_FAILED)));
  EXPECT_EQ(0, test_handler_->total_bytes_downloaded());
  EXPECT_FALSE(test_handler_->final_status().is_success());
  EXPECT_EQ(net::ERR_FAILED, test_handler_->final_status().error());
}

TEST_P(RedirectToFileResourceHandlerTest, SecondWriteFails) {
  const int kFirstWriteSize = kMaxInitialSyncReadSize;
  std::string test_data =
      CreateTestData(RedirectToFileResourceHandler::kInitialReadBufSize);
  file_stream_->set_expected_written_data(test_data.substr(0, kFirstWriteSize));
  file_stream_->set_all_write_results(
      MockFileStream::OperationResult(net::ERR_FAILED, GetParam()));
  file_stream_->set_next_write_result(MockFileStream::OperationResult(
      std::numeric_limits<int>::max(), GetParam()));

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            StartAndCreateStream(base::File::FILE_OK));
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            OnResponseStartedAndWaitForResult());

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(
                base::StringPiece(test_data.data(), kFirstWriteSize)));
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  mock_loader_->OnReadCompleted(base::StringPiece(
      test_data.data() + kFirstWriteSize, test_data.size() - kFirstWriteSize));
  // Wait for the write to complete, in the async case.
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(MockResourceLoader::Status::CANCELED, mock_loader_->status());

  EXPECT_EQ(net::ERR_FAILED, mock_loader_->error_code());
  EXPECT_EQ(0, test_handler_->on_response_completed_called());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseCompleted(
                net::URLRequestStatus::FromError(net::ERR_FAILED)));
  EXPECT_EQ(kFirstWriteSize, test_handler_->total_bytes_downloaded());
  EXPECT_FALSE(test_handler_->final_status().is_success());
  EXPECT_EQ(net::ERR_FAILED, test_handler_->final_status().error());
}

INSTANTIATE_TEST_CASE_P(/* No prefix needed */,
                        RedirectToFileResourceHandlerTest,
                        testing::Values(CompletionMode::SYNC,
                                        CompletionMode::ASYNC));

}  // namespace
}  // namespace content
