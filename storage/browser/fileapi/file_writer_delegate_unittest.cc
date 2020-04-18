// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/scoped_temp_dir.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/io_buffer.h"
#include "net/base/request_priority.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_job.h"
#include "net/url_request/url_request_job_factory.h"
#include "net/url_request/url_request_status.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_quota_util.h"
#include "storage/browser/fileapi/file_writer_delegate.h"
#include "storage/browser/fileapi/sandbox_file_stream_writer.h"
#include "storage/browser/test/async_file_test_helper.h"
#include "storage/browser/test/test_file_system_context.h"
#include "storage/common/fileapi/file_system_mount_option.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

using content::AsyncFileTestHelper;
using storage::FileSystemURL;
using storage::FileWriterDelegate;

namespace content {

namespace {

const GURL kOrigin("http://example.com");
const storage::FileSystemType kFileSystemType = storage::kFileSystemTypeTest;

const char kData[] = "The quick brown fox jumps over the lazy dog.\n";
const int kDataSize = arraysize(kData) - 1;

class Result {
 public:
  Result()
      : status_(base::File::FILE_OK),
        bytes_written_(0),
        write_status_(FileWriterDelegate::SUCCESS_IO_PENDING) {}

  base::File::Error status() const { return status_; }
  int64_t bytes_written() const { return bytes_written_; }
  FileWriterDelegate::WriteProgressStatus write_status() const {
    return write_status_;
  }

  void DidWrite(base::File::Error status,
                int64_t bytes,
                FileWriterDelegate::WriteProgressStatus write_status) {
    write_status_ = write_status;
    if (status == base::File::FILE_OK) {
      bytes_written_ += bytes;
      if (write_status_ != FileWriterDelegate::SUCCESS_IO_PENDING)
        base::RunLoop::QuitCurrentWhenIdleDeprecated();
    } else {
      EXPECT_EQ(base::File::FILE_OK, status_);
      status_ = status;
      base::RunLoop::QuitCurrentWhenIdleDeprecated();
    }
  }

 private:
  // For post-operation status.
  base::File::Error status_;
  int64_t bytes_written_;
  FileWriterDelegate::WriteProgressStatus write_status_;
};

class BlobURLRequestJobFactory;

}  // namespace (anonymous)

class FileWriterDelegateTest : public PlatformTest {
 public:
  FileWriterDelegateTest() = default;

 protected:
  void SetUp() override;
  void TearDown() override;

  int64_t usage() {
    return file_system_context_->GetQuotaUtil(kFileSystemType)
        ->GetOriginUsageOnFileTaskRunner(
              file_system_context_.get(), kOrigin, kFileSystemType);
  }

  int64_t GetFileSizeOnDisk(const char* test_file_path) {
    // There might be in-flight flush/write.
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, base::DoNothing());
    base::RunLoop().RunUntilIdle();

    FileSystemURL url = GetFileSystemURL(test_file_path);
    base::File::Info file_info;
    EXPECT_EQ(base::File::FILE_OK,
              AsyncFileTestHelper::GetMetadata(
                  file_system_context_.get(), url, &file_info));
    return file_info.size;
  }

  FileSystemURL GetFileSystemURL(const char* file_name) const {
    return file_system_context_->CreateCrackedFileSystemURL(
        kOrigin, kFileSystemType, base::FilePath().FromUTF8Unsafe(file_name));
  }

  std::unique_ptr<storage::SandboxFileStreamWriter> CreateWriter(
      const char* test_file_path,
      int64_t offset,
      int64_t allowed_growth) {
    auto writer = std::make_unique<storage::SandboxFileStreamWriter>(
        file_system_context_.get(), GetFileSystemURL(test_file_path), offset,
        *file_system_context_->GetUpdateObservers(kFileSystemType));
    writer->set_default_quota(allowed_growth);
    return writer;
  }

  std::unique_ptr<FileWriterDelegate> CreateWriterDelegate(
      const char* test_file_path,
      int64_t offset,
      int64_t allowed_growth) {
    auto writer = CreateWriter(test_file_path, offset, allowed_growth);
    return std::make_unique<FileWriterDelegate>(
        std::move(writer), storage::FlushPolicy::FLUSH_ON_COMPLETION);
  }

  FileWriterDelegate::DelegateWriteCallback GetWriteCallback(Result* result) {
    return base::BindRepeating(&Result::DidWrite, base::Unretained(result));
  }

  // Creates and sets up a FileWriterDelegate for writing the given |blob_url|,
  // and creates a new FileWriterDelegate for the file.
  void PrepareForWrite(const char* test_file_path,
                       const GURL& blob_url,
                       int64_t offset,
                       int64_t allowed_growth) {
    file_writer_delegate_ =
        CreateWriterDelegate(test_file_path, offset, allowed_growth);
    request_ = empty_context_.CreateRequest(blob_url, net::DEFAULT_PRIORITY,
                                            file_writer_delegate_.get(),
                                            TRAFFIC_ANNOTATION_FOR_TESTS);
  }

  // This should be alive until the very end of this instance.
  base::MessageLoopForIO loop_;

  scoped_refptr<storage::FileSystemContext> file_system_context_;

  net::URLRequestContext empty_context_;
  std::unique_ptr<FileWriterDelegate> file_writer_delegate_;
  std::unique_ptr<net::URLRequest> request_;
  std::unique_ptr<BlobURLRequestJobFactory> job_factory_;

  base::ScopedTempDir dir_;

  static const char* content_;
};

const char* FileWriterDelegateTest::content_ = NULL;

namespace {

static std::string g_content;

class FileWriterDelegateTestJob : public net::URLRequestJob {
 public:
  FileWriterDelegateTestJob(net::URLRequest* request,
                            net::NetworkDelegate* network_delegate,
                            const std::string& content)
      : net::URLRequestJob(request, network_delegate),
        content_(content),
        remaining_bytes_(content.length()),
        cursor_(0),
        weak_factory_(this) {}

  void Start() override {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&FileWriterDelegateTestJob::NotifyHeadersComplete,
                       weak_factory_.GetWeakPtr()));
  }

  int ReadRawData(net::IOBuffer* buf, int buf_size) override {
    if (remaining_bytes_ < buf_size)
      buf_size = remaining_bytes_;

    for (int i = 0; i < buf_size; ++i)
      buf->data()[i] = content_[cursor_++];
    remaining_bytes_ -= buf_size;

    return buf_size;
  }

  void GetResponseInfo(net::HttpResponseInfo* info) override {
    const char kStatus[] = "HTTP/1.1 200 OK\0";
    const size_t kStatusLen = arraysize(kStatus);

    info->headers =
        new net::HttpResponseHeaders(std::string(kStatus, kStatusLen));
  }

 protected:
  ~FileWriterDelegateTestJob() override = default;

 private:
  std::string content_;
  int remaining_bytes_;
  int cursor_;

  base::WeakPtrFactory<FileWriterDelegateTestJob> weak_factory_;
};

class BlobURLRequestJobFactory : public net::URLRequestJobFactory {
 public:
  explicit BlobURLRequestJobFactory(const char** content_data)
      : content_data_(content_data) {
  }

  net::URLRequestJob* MaybeCreateJobWithProtocolHandler(
      const std::string& scheme,
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override {
    return new FileWriterDelegateTestJob(request, network_delegate,
                                         *content_data_);
  }

  net::URLRequestJob* MaybeInterceptRedirect(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      const GURL& location) const override {
    return nullptr;
  }

  net::URLRequestJob* MaybeInterceptResponse(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override {
    return nullptr;
  }

  bool IsHandledProtocol(const std::string& scheme) const override {
    return scheme == "blob";
  }

  bool IsSafeRedirectTarget(const GURL& location) const override {
    return true;
  }

 private:
  const char** content_data_;

  DISALLOW_COPY_AND_ASSIGN(BlobURLRequestJobFactory);
};

}  // namespace (anonymous)

void FileWriterDelegateTest::SetUp() {
  ASSERT_TRUE(dir_.CreateUniqueTempDir());

  file_system_context_ =
      CreateFileSystemContextForTesting(NULL, dir_.GetPath());
  ASSERT_EQ(base::File::FILE_OK,
            AsyncFileTestHelper::CreateFile(file_system_context_.get(),
                                            GetFileSystemURL("test")));
  job_factory_ = std::make_unique<BlobURLRequestJobFactory>(&content_);
  empty_context_.set_job_factory(job_factory_.get());
}

void FileWriterDelegateTest::TearDown() {
  file_system_context_ = NULL;
  base::RunLoop().RunUntilIdle();
}

TEST_F(FileWriterDelegateTest, WriteSuccessWithoutQuotaLimit) {
  const GURL kBlobURL("blob:nolimit");
  content_ = kData;

  PrepareForWrite("test", kBlobURL, 0, std::numeric_limits<int64_t>::max());

  Result result;
  ASSERT_EQ(0, usage());
  file_writer_delegate_->Start(std::move(request_), GetWriteCallback(&result));
  base::RunLoop().Run();

  ASSERT_EQ(FileWriterDelegate::SUCCESS_COMPLETED, result.write_status());
  file_writer_delegate_.reset();

  ASSERT_EQ(kDataSize, usage());
  EXPECT_EQ(GetFileSizeOnDisk("test"), usage());
  EXPECT_EQ(kDataSize, result.bytes_written());
  EXPECT_EQ(base::File::FILE_OK, result.status());
}

TEST_F(FileWriterDelegateTest, WriteSuccessWithJustQuota) {
  const GURL kBlobURL("blob:just");
  content_ = kData;
  const int64_t kAllowedGrowth = kDataSize;
  PrepareForWrite("test", kBlobURL, 0, kAllowedGrowth);

  Result result;
  ASSERT_EQ(0, usage());
  file_writer_delegate_->Start(std::move(request_), GetWriteCallback(&result));
  base::RunLoop().Run();
  ASSERT_EQ(FileWriterDelegate::SUCCESS_COMPLETED, result.write_status());
  file_writer_delegate_.reset();

  ASSERT_EQ(kAllowedGrowth, usage());
  EXPECT_EQ(GetFileSizeOnDisk("test"), usage());

  EXPECT_EQ(kAllowedGrowth, result.bytes_written());
  EXPECT_EQ(base::File::FILE_OK, result.status());
}

TEST_F(FileWriterDelegateTest, DISABLED_WriteFailureByQuota) {
  const GURL kBlobURL("blob:failure");
  content_ = kData;
  const int64_t kAllowedGrowth = kDataSize - 1;
  PrepareForWrite("test", kBlobURL, 0, kAllowedGrowth);

  Result result;
  ASSERT_EQ(0, usage());
  file_writer_delegate_->Start(std::move(request_), GetWriteCallback(&result));
  base::RunLoop().Run();
  ASSERT_EQ(FileWriterDelegate::ERROR_WRITE_STARTED, result.write_status());
  file_writer_delegate_.reset();

  ASSERT_EQ(kAllowedGrowth, usage());
  EXPECT_EQ(GetFileSizeOnDisk("test"), usage());

  EXPECT_EQ(kAllowedGrowth, result.bytes_written());
  EXPECT_EQ(base::File::FILE_ERROR_NO_SPACE, result.status());
  ASSERT_EQ(FileWriterDelegate::ERROR_WRITE_STARTED, result.write_status());
}

TEST_F(FileWriterDelegateTest, WriteZeroBytesSuccessfullyWithZeroQuota) {
  const GURL kBlobURL("blob:zero");
  content_ = "";
  int64_t kAllowedGrowth = 0;
  PrepareForWrite("test", kBlobURL, 0, kAllowedGrowth);

  Result result;
  ASSERT_EQ(0, usage());
  file_writer_delegate_->Start(std::move(request_), GetWriteCallback(&result));
  base::RunLoop().Run();
  ASSERT_EQ(FileWriterDelegate::SUCCESS_COMPLETED, result.write_status());
  file_writer_delegate_.reset();

  ASSERT_EQ(kAllowedGrowth, usage());
  EXPECT_EQ(GetFileSizeOnDisk("test"), usage());

  EXPECT_EQ(kAllowedGrowth, result.bytes_written());
  EXPECT_EQ(base::File::FILE_OK, result.status());
  ASSERT_EQ(FileWriterDelegate::SUCCESS_COMPLETED, result.write_status());
}

TEST_F(FileWriterDelegateTest, WriteSuccessWithoutQuotaLimitConcurrent) {
  std::unique_ptr<FileWriterDelegate> file_writer_delegate2;
  std::unique_ptr<net::URLRequest> request2;

  ASSERT_EQ(base::File::FILE_OK,
            AsyncFileTestHelper::CreateFile(file_system_context_.get(),
                                            GetFileSystemURL("test2")));

  const GURL kBlobURL("blob:nolimitconcurrent");
  const GURL kBlobURL2("blob:nolimitconcurrent2");
  content_ = kData;

  PrepareForWrite("test", kBlobURL, 0, std::numeric_limits<int64_t>::max());

  // Create another FileWriterDelegate for concurrent write.
  file_writer_delegate2 =
      CreateWriterDelegate("test2", 0, std::numeric_limits<int64_t>::max());
  request2 = empty_context_.CreateRequest(kBlobURL2, net::DEFAULT_PRIORITY,
                                          file_writer_delegate2.get(),
                                          TRAFFIC_ANNOTATION_FOR_TESTS);

  Result result, result2;
  ASSERT_EQ(0, usage());
  file_writer_delegate_->Start(std::move(request_), GetWriteCallback(&result));
  file_writer_delegate2->Start(std::move(request2), GetWriteCallback(&result2));
  base::RunLoop().Run();
  if (result.write_status() == FileWriterDelegate::SUCCESS_IO_PENDING ||
      result2.write_status() == FileWriterDelegate::SUCCESS_IO_PENDING)
    base::RunLoop().Run();

  ASSERT_EQ(FileWriterDelegate::SUCCESS_COMPLETED, result.write_status());
  ASSERT_EQ(FileWriterDelegate::SUCCESS_COMPLETED, result2.write_status());
  file_writer_delegate_.reset();
  file_writer_delegate2.reset();

  ASSERT_EQ(kDataSize * 2, usage());
  EXPECT_EQ(GetFileSizeOnDisk("test") + GetFileSizeOnDisk("test2"), usage());

  EXPECT_EQ(kDataSize, result.bytes_written());
  EXPECT_EQ(base::File::FILE_OK, result.status());
  EXPECT_EQ(kDataSize, result2.bytes_written());
  EXPECT_EQ(base::File::FILE_OK, result2.status());
}

TEST_F(FileWriterDelegateTest, WritesWithQuotaAndOffset) {
  const GURL kBlobURL("blob:failure-with-updated-quota");
  content_ = kData;

  // Writing kDataSize (=45) bytes data while allowed_growth is 100.
  int64_t offset = 0;
  int64_t allowed_growth = 100;
  ASSERT_LT(kDataSize, allowed_growth);
  PrepareForWrite("test", kBlobURL, offset, allowed_growth);

  {
    Result result;
    ASSERT_EQ(0, usage());
    file_writer_delegate_->Start(std::move(request_),
                                 GetWriteCallback(&result));
    base::RunLoop().Run();
    ASSERT_EQ(FileWriterDelegate::SUCCESS_COMPLETED, result.write_status());
    file_writer_delegate_.reset();

    ASSERT_EQ(kDataSize, usage());
    EXPECT_EQ(GetFileSizeOnDisk("test"), usage());
    EXPECT_EQ(kDataSize, result.bytes_written());
    EXPECT_EQ(base::File::FILE_OK, result.status());
  }

  // Trying to overwrite kDataSize bytes data while allowed_growth is 20.
  offset = 0;
  allowed_growth = 20;
  PrepareForWrite("test", kBlobURL, offset, allowed_growth);

  {
    Result result;
    file_writer_delegate_->Start(std::move(request_),
                                 GetWriteCallback(&result));
    base::RunLoop().Run();
    EXPECT_EQ(kDataSize, usage());
    EXPECT_EQ(GetFileSizeOnDisk("test"), usage());
    EXPECT_EQ(kDataSize, result.bytes_written());
    EXPECT_EQ(base::File::FILE_OK, result.status());
    ASSERT_EQ(FileWriterDelegate::SUCCESS_COMPLETED, result.write_status());
  }

  // Trying to write kDataSize bytes data from offset 25 while
  // allowed_growth is 55.
  offset = 25;
  allowed_growth = 55;
  PrepareForWrite("test", kBlobURL, offset, allowed_growth);

  {
    Result result;
    file_writer_delegate_->Start(std::move(request_),
                                 GetWriteCallback(&result));
    base::RunLoop().Run();
    ASSERT_EQ(FileWriterDelegate::SUCCESS_COMPLETED, result.write_status());
    file_writer_delegate_.reset();

    EXPECT_EQ(offset + kDataSize, usage());
    EXPECT_EQ(GetFileSizeOnDisk("test"), usage());
    EXPECT_EQ(kDataSize, result.bytes_written());
    EXPECT_EQ(base::File::FILE_OK, result.status());
  }

  // Trying to overwrite 45 bytes data while allowed_growth is -20.
  offset = 0;
  allowed_growth = -20;
  PrepareForWrite("test", kBlobURL, offset, allowed_growth);
  int64_t pre_write_usage = GetFileSizeOnDisk("test");

  {
    Result result;
    file_writer_delegate_->Start(std::move(request_),
                                 GetWriteCallback(&result));
    base::RunLoop().Run();
    ASSERT_EQ(FileWriterDelegate::SUCCESS_COMPLETED, result.write_status());
    file_writer_delegate_.reset();

    EXPECT_EQ(pre_write_usage, usage());
    EXPECT_EQ(GetFileSizeOnDisk("test"), usage());
    EXPECT_EQ(kDataSize, result.bytes_written());
    EXPECT_EQ(base::File::FILE_OK, result.status());
  }

  // Trying to overwrite 45 bytes data with offset pre_write_usage - 20,
  // while allowed_growth is 10.
  const int kOverlap = 20;
  offset = pre_write_usage - kOverlap;
  allowed_growth = 10;
  PrepareForWrite("test", kBlobURL, offset, allowed_growth);

  {
    Result result;
    file_writer_delegate_->Start(std::move(request_),
                                 GetWriteCallback(&result));
    base::RunLoop().Run();
    ASSERT_EQ(FileWriterDelegate::ERROR_WRITE_STARTED, result.write_status());
    file_writer_delegate_.reset();

    EXPECT_EQ(pre_write_usage + allowed_growth, usage());
    EXPECT_EQ(GetFileSizeOnDisk("test"), usage());
    EXPECT_EQ(kOverlap + allowed_growth, result.bytes_written());
    EXPECT_EQ(base::File::FILE_ERROR_NO_SPACE, result.status());
  }
}

class InterruptedFileWriterDelegate : public FileWriterDelegate {
 public:
  InterruptedFileWriterDelegate(
      std::unique_ptr<storage::FileStreamWriter> file_writer,
      storage::FlushPolicy flush_policy)
      : FileWriterDelegate(std::move(file_writer), flush_policy) {}
  ~InterruptedFileWriterDelegate() override = default;

  void OnDataReceived(int bytes_read) override {
    // The base class will respond to OnDataReceived by performing an
    // asynchronous write. Schedule a task now that will execute before the
    // write completes which terminates the URLRequestJob.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(IgnoreResult(&net::URLRequest::Cancel),
                                  base::Unretained(request_)));
    FileWriterDelegate::OnDataReceived(bytes_read);
  }

  void set_request(net::URLRequest* request) { request_ = request; }

 private:
  // The request is owned by the base class as a private member.
  net::URLRequest* request_ = nullptr;
};

TEST_F(FileWriterDelegateTest, ReadFailureDuringAsyncWrite) {
  const GURL kBlobURL("blob:async-fail");
  content_ = kData;

  auto writer = CreateWriter("test", 0, std::numeric_limits<int64_t>::max());
  auto file_writer_delegate = std::make_unique<InterruptedFileWriterDelegate>(
      std::move(writer), storage::FlushPolicy::FLUSH_ON_COMPLETION);
  auto request = empty_context_.CreateRequest(kBlobURL, net::DEFAULT_PRIORITY,
                                              file_writer_delegate.get(),
                                              TRAFFIC_ANNOTATION_FOR_TESTS);
  file_writer_delegate->set_request(request.get());

  Result result;
  file_writer_delegate->Start(std::move(request), GetWriteCallback(&result));
  base::RunLoop().Run();

  ASSERT_EQ(FileWriterDelegate::ERROR_WRITE_STARTED, result.write_status());
  file_writer_delegate_.reset();

  // The write should still have flushed.
  ASSERT_EQ(kDataSize, usage());
  EXPECT_EQ(GetFileSizeOnDisk("test"), usage());
}

}  // namespace content
