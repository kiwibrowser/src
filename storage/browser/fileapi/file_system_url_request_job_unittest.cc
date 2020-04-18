// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/format_macros.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/rand_util.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/load_flags.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/base/request_priority.h"
#include "net/http/http_byte_range.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_test_util.h"
#include "storage/browser/fileapi/external_mount_points.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_file_util.h"
#include "storage/browser/fileapi/file_system_url_request_job.h"
#include "storage/browser/test/async_file_test_helper.h"
#include "storage/browser/test/test_file_system_backend.h"
#include "storage/browser/test/test_file_system_context.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::AsyncFileTestHelper;
using storage::FileSystemContext;
using storage::FileSystemURL;
using storage::FileSystemURLRequestJob;

namespace content {
namespace {

// We always use the TEMPORARY FileSystem in this test.
const char kFileSystemURLPrefix[] = "filesystem:http://remote/temporary/";
const char kTestFileData[] = "0123456789";

void FillBuffer(char* buffer, size_t len) {
  base::RandBytes(buffer, len);
}

const char kValidExternalMountPoint[] = "mnt_name";

// An auto mounter that will try to mount anything for |storage_domain| =
// "automount", but will only succeed for the mount point "mnt_name".
bool TestAutoMountForURLRequest(
    const storage::FileSystemRequestInfo& request_info,
    const storage::FileSystemURL& filesystem_url,
    base::OnceCallback<void(base::File::Error result)> callback) {
  if (request_info.storage_domain != "automount")
    return false;
  std::vector<base::FilePath::StringType> components;
  filesystem_url.path().GetComponents(&components);
  std::string mount_point = base::FilePath(components[0]).AsUTF8Unsafe();

  if (mount_point == kValidExternalMountPoint) {
    storage::ExternalMountPoints::GetSystemInstance()->RegisterFileSystem(
        kValidExternalMountPoint,
        storage::kFileSystemTypeTest,
        storage::FileSystemMountOption(),
        base::FilePath());
    std::move(callback).Run(base::File::FILE_OK);
  } else {
    std::move(callback).Run(base::File::FILE_ERROR_NOT_FOUND);
  }
  return true;
}

class FileSystemURLRequestJobFactory : public net::URLRequestJobFactory {
 public:
  FileSystemURLRequestJobFactory(const std::string& storage_domain,
                                 FileSystemContext* context)
      : storage_domain_(storage_domain), file_system_context_(context) {
  }

  net::URLRequestJob* MaybeCreateJobWithProtocolHandler(
      const std::string& scheme,
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override {
    return new storage::FileSystemURLRequestJob(
        request, network_delegate, storage_domain_, file_system_context_);
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
    return true;
  }

  bool IsSafeRedirectTarget(const GURL& location) const override {
    return false;
  }

 private:
  std::string storage_domain_;
  FileSystemContext* file_system_context_;
};

}  // namespace

class FileSystemURLRequestJobTest : public testing::Test {
 protected:
  FileSystemURLRequestJobTest() : weak_factory_(this) {
  }

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    // We use the main thread so that we can get the root path synchronously.
    // TODO(adamk): Run this on the FILE thread we've created as well.
    file_system_context_ =
        CreateFileSystemContextForTesting(NULL, temp_dir_.GetPath());

    file_system_context_->OpenFileSystem(
        GURL("http://remote/"), storage::kFileSystemTypeTemporary,
        storage::OPEN_FILE_SYSTEM_CREATE_IF_NONEXISTENT,
        base::BindOnce(&FileSystemURLRequestJobTest::OnOpenFileSystem,
                       weak_factory_.GetWeakPtr()));
    base::RunLoop().RunUntilIdle();
  }

  void TearDown() override {
    // FileReader posts a task to close the file in destructor.
    base::RunLoop().RunUntilIdle();
  }

  void SetUpAutoMountContext() {
    base::FilePath mnt_point =
        temp_dir_.GetPath().AppendASCII("auto_mount_dir");
    ASSERT_TRUE(base::CreateDirectory(mnt_point));

    std::vector<std::unique_ptr<storage::FileSystemBackend>>
        additional_providers;
    additional_providers.push_back(std::make_unique<TestFileSystemBackend>(
        base::ThreadTaskRunnerHandle::Get().get(), mnt_point));

    std::vector<storage::URLRequestAutoMountHandler> handlers;
    handlers.push_back(base::BindRepeating(&TestAutoMountForURLRequest));

    file_system_context_ = CreateFileSystemContextWithAutoMountersForTesting(
        NULL, std::move(additional_providers), handlers, temp_dir_.GetPath());

    ASSERT_EQ(static_cast<int>(sizeof(kTestFileData)) - 1,
              base::WriteFile(mnt_point.AppendASCII("foo"), kTestFileData,
                              sizeof(kTestFileData) - 1));
  }

  void OnOpenFileSystem(const GURL& root_url,
                        const std::string& name,
                        base::File::Error result) {
    ASSERT_EQ(base::File::FILE_OK, result);
  }

  void TestRequestHelper(const GURL& url,
                         const net::HttpRequestHeaders* headers,
                         bool run_to_completion,
                         FileSystemContext* file_system_context) {
    delegate_.reset(new net::TestDelegate());
    // Make delegate_ exit the MessageLoop when the request is done.
    delegate_->set_quit_on_complete(true);
    delegate_->set_quit_on_redirect(true);

    job_factory_.reset(new FileSystemURLRequestJobFactory(
        url.GetOrigin().host(), file_system_context));
    empty_context_.set_job_factory(job_factory_.get());

    request_ = empty_context_.CreateRequest(url, net::DEFAULT_PRIORITY,
                                            delegate_.get(),
                                            TRAFFIC_ANNOTATION_FOR_TESTS);
    if (headers)
      request_->SetExtraRequestHeaders(*headers);

    request_->Start();
    ASSERT_TRUE(request_->is_pending());  // verify that we're starting async
    if (run_to_completion)
      base::RunLoop().Run();
  }

  void TestRequest(const GURL& url) {
    TestRequestHelper(url, NULL, true, file_system_context_.get());
  }

  void TestRequestWithContext(const GURL& url,
                              FileSystemContext* file_system_context) {
    TestRequestHelper(url, NULL, true, file_system_context);
  }

  void TestRequestWithHeaders(const GURL& url,
                              const net::HttpRequestHeaders* headers) {
    TestRequestHelper(url, headers, true, file_system_context_.get());
  }

  void TestRequestNoRun(const GURL& url) {
    TestRequestHelper(url, NULL, false, file_system_context_.get());
  }

  void CreateDirectory(const base::StringPiece& dir_name) {
    FileSystemURL url = file_system_context_->CreateCrackedFileSystemURL(
        GURL("http://remote"),
        storage::kFileSystemTypeTemporary,
        base::FilePath().AppendASCII(dir_name));
    ASSERT_EQ(
        base::File::FILE_OK,
        AsyncFileTestHelper::CreateDirectory(file_system_context_.get(), url));
  }

  void WriteFile(const base::StringPiece& file_name,
                 const char* buf, int buf_size) {
    FileSystemURL url = file_system_context_->CreateCrackedFileSystemURL(
        GURL("http://remote"),
        storage::kFileSystemTypeTemporary,
        base::FilePath().AppendASCII(file_name));
    ASSERT_EQ(base::File::FILE_OK,
              AsyncFileTestHelper::CreateFileWithData(
                  file_system_context_.get(), url, buf, buf_size));
  }

  GURL CreateFileSystemURL(const std::string& path) {
    return GURL(kFileSystemURLPrefix + path);
  }

  // Temp directory is at the top because it must be deleted last.
  base::ScopedTempDir temp_dir_;

  // The message loop must be deleted second to last.
  base::MessageLoopForIO message_loop_;

  scoped_refptr<storage::FileSystemContext> file_system_context_;

  net::URLRequestContext empty_context_;
  std::unique_ptr<FileSystemURLRequestJobFactory> job_factory_;

  // NOTE: order matters, request must die before delegate
  std::unique_ptr<net::TestDelegate> delegate_;
  std::unique_ptr<net::URLRequest> request_;

  base::WeakPtrFactory<FileSystemURLRequestJobTest> weak_factory_;
};

namespace {

TEST_F(FileSystemURLRequestJobTest, FileTest) {
  WriteFile("file1.dat", kTestFileData, arraysize(kTestFileData) - 1);
  TestRequest(CreateFileSystemURL("file1.dat"));

  ASSERT_FALSE(request_->is_pending());
  EXPECT_EQ(1, delegate_->response_started_count());
  EXPECT_FALSE(delegate_->received_data_before_response());
  EXPECT_EQ(kTestFileData, delegate_->data_received());
  EXPECT_EQ(200, request_->GetResponseCode());
  std::string cache_control;
  request_->GetResponseHeaderByName("cache-control", &cache_control);
  EXPECT_EQ("no-cache", cache_control);
}

TEST_F(FileSystemURLRequestJobTest, FileTestFullSpecifiedRange) {
  const size_t buffer_size = 4000;
  std::unique_ptr<char[]> buffer(new char[buffer_size]);
  FillBuffer(buffer.get(), buffer_size);
  WriteFile("bigfile", buffer.get(), buffer_size);

  const size_t first_byte_position = 500;
  const size_t last_byte_position = buffer_size - first_byte_position;
  std::string partial_buffer_string(buffer.get() + first_byte_position,
                                    buffer.get() + last_byte_position + 1);

  net::HttpRequestHeaders headers;
  headers.SetHeader(
      net::HttpRequestHeaders::kRange,
      net::HttpByteRange::Bounded(
          first_byte_position, last_byte_position).GetHeaderValue());
  TestRequestWithHeaders(CreateFileSystemURL("bigfile"), &headers);

  ASSERT_FALSE(request_->is_pending());
  EXPECT_EQ(1, delegate_->response_started_count());
  EXPECT_FALSE(delegate_->received_data_before_response());
  EXPECT_TRUE(partial_buffer_string == delegate_->data_received());
}

TEST_F(FileSystemURLRequestJobTest, FileTestHalfSpecifiedRange) {
  const size_t buffer_size = 4000;
  std::unique_ptr<char[]> buffer(new char[buffer_size]);
  FillBuffer(buffer.get(), buffer_size);
  WriteFile("bigfile", buffer.get(), buffer_size);

  const size_t first_byte_position = 500;
  std::string partial_buffer_string(buffer.get() + first_byte_position,
                                    buffer.get() + buffer_size);

  net::HttpRequestHeaders headers;
  headers.SetHeader(
      net::HttpRequestHeaders::kRange,
      net::HttpByteRange::RightUnbounded(first_byte_position).GetHeaderValue());
  TestRequestWithHeaders(CreateFileSystemURL("bigfile"), &headers);
  ASSERT_FALSE(request_->is_pending());
  EXPECT_EQ(1, delegate_->response_started_count());
  EXPECT_FALSE(delegate_->received_data_before_response());
  // Don't use EXPECT_EQ, it will print out a lot of garbage if check failed.
  EXPECT_TRUE(partial_buffer_string == delegate_->data_received());
}

TEST_F(FileSystemURLRequestJobTest, FileTestMultipleRangesNotSupported) {
  WriteFile("file1.dat", kTestFileData, arraysize(kTestFileData) - 1);
  net::HttpRequestHeaders headers;
  headers.SetHeader(net::HttpRequestHeaders::kRange,
                    "bytes=0-5,10-200,200-300");
  TestRequestWithHeaders(CreateFileSystemURL("file1.dat"), &headers);
  EXPECT_TRUE(delegate_->request_failed());
  EXPECT_EQ(net::ERR_REQUEST_RANGE_NOT_SATISFIABLE,
            delegate_->request_status());
}

TEST_F(FileSystemURLRequestJobTest, RangeOutOfBounds) {
  WriteFile("file1.dat", kTestFileData, arraysize(kTestFileData) - 1);
  net::HttpRequestHeaders headers;
  headers.SetHeader(
      net::HttpRequestHeaders::kRange,
      net::HttpByteRange::Bounded(500, 1000).GetHeaderValue());
  TestRequestWithHeaders(CreateFileSystemURL("file1.dat"), &headers);

  ASSERT_FALSE(request_->is_pending());
  EXPECT_TRUE(delegate_->request_failed());
  EXPECT_EQ(net::ERR_REQUEST_RANGE_NOT_SATISFIABLE,
            delegate_->request_status());
}

TEST_F(FileSystemURLRequestJobTest, FileDirRedirect) {
  CreateDirectory("dir");
  TestRequest(CreateFileSystemURL("dir"));

  EXPECT_EQ(1, delegate_->received_redirect_count());
  EXPECT_FALSE(delegate_->request_failed());

  // We've deferred the redirect; now cancel the request to avoid following it.
  request_->Cancel();
  base::RunLoop().Run();
}

TEST_F(FileSystemURLRequestJobTest, InvalidURL) {
  TestRequest(GURL("filesystem:/foo/bar/baz"));
  ASSERT_FALSE(request_->is_pending());
  EXPECT_TRUE(delegate_->request_failed());
  EXPECT_EQ(net::ERR_INVALID_URL, delegate_->request_status());
}

TEST_F(FileSystemURLRequestJobTest, NoSuchRoot) {
  TestRequest(GURL("filesystem:http://remote/persistent/somefile"));
  ASSERT_FALSE(request_->is_pending());
  EXPECT_TRUE(delegate_->request_failed());
  EXPECT_EQ(net::ERR_FILE_NOT_FOUND, delegate_->request_status());
}

TEST_F(FileSystemURLRequestJobTest, NoSuchFile) {
  TestRequest(CreateFileSystemURL("somefile"));
  ASSERT_FALSE(request_->is_pending());
  EXPECT_TRUE(delegate_->request_failed());
  EXPECT_EQ(net::ERR_FILE_NOT_FOUND, delegate_->request_status());
}

TEST_F(FileSystemURLRequestJobTest, Cancel) {
  WriteFile("file1.dat", kTestFileData, arraysize(kTestFileData) - 1);
  TestRequestNoRun(CreateFileSystemURL("file1.dat"));

  // Run StartAsync() and only StartAsync().
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE,
                                                  request_.release());
  base::RunLoop().RunUntilIdle();
  // If we get here, success! we didn't crash!
}

TEST_F(FileSystemURLRequestJobTest, GetMimeType) {
  const char kFilename[] = "hoge.html";

  std::string mime_type_direct;
  base::FilePath::StringType extension =
      base::FilePath().AppendASCII(kFilename).Extension();
  if (!extension.empty())
    extension = extension.substr(1);
  EXPECT_TRUE(net::GetWellKnownMimeTypeFromExtension(
      extension, &mime_type_direct));

  TestRequest(CreateFileSystemURL(kFilename));

  std::string mime_type_from_job;
  request_->GetMimeType(&mime_type_from_job);
  EXPECT_EQ(mime_type_direct, mime_type_from_job);
}

TEST_F(FileSystemURLRequestJobTest, Incognito) {
  WriteFile("file", kTestFileData, arraysize(kTestFileData) - 1);

  // Creates a new filesystem context for incognito mode.
  scoped_refptr<FileSystemContext> file_system_context =
      CreateIncognitoFileSystemContextForTesting(NULL, temp_dir_.GetPath());

  // The request should return NOT_FOUND error if it's in incognito mode.
  TestRequestWithContext(CreateFileSystemURL("file"),
                         file_system_context.get());
  ASSERT_FALSE(request_->is_pending());
  EXPECT_TRUE(delegate_->request_failed());
  EXPECT_EQ(net::ERR_FILE_NOT_FOUND, delegate_->request_status());

  // Make sure it returns success with regular (non-incognito) context.
  TestRequest(CreateFileSystemURL("file"));
  ASSERT_FALSE(request_->is_pending());
  EXPECT_EQ(kTestFileData, delegate_->data_received());
  EXPECT_EQ(200, request_->GetResponseCode());
}

TEST_F(FileSystemURLRequestJobTest, AutoMountFileTest) {
  SetUpAutoMountContext();
  TestRequest(GURL("filesystem:http://automount/external/mnt_name/foo"));

  ASSERT_FALSE(request_->is_pending());
  EXPECT_EQ(1, delegate_->response_started_count());
  EXPECT_FALSE(delegate_->received_data_before_response());
  EXPECT_EQ(kTestFileData, delegate_->data_received());
  EXPECT_EQ(200, request_->GetResponseCode());
  std::string cache_control;
  request_->GetResponseHeaderByName("cache-control", &cache_control);
  EXPECT_EQ("no-cache", cache_control);

  ASSERT_TRUE(
      storage::ExternalMountPoints::GetSystemInstance()->RevokeFileSystem(
          kValidExternalMountPoint));
}

TEST_F(FileSystemURLRequestJobTest, AutoMountInvalidRoot) {
  SetUpAutoMountContext();
  TestRequest(GURL("filesystem:http://automount/external/invalid/foo"));

  ASSERT_FALSE(request_->is_pending());
  EXPECT_TRUE(delegate_->request_failed());
  EXPECT_EQ(net::ERR_FILE_NOT_FOUND, delegate_->request_status());

  ASSERT_FALSE(
      storage::ExternalMountPoints::GetSystemInstance()->RevokeFileSystem(
          "invalid"));
}

TEST_F(FileSystemURLRequestJobTest, AutoMountNoHandler) {
  SetUpAutoMountContext();
  TestRequest(GURL("filesystem:http://noauto/external/mnt_name/foo"));

  ASSERT_FALSE(request_->is_pending());
  EXPECT_TRUE(delegate_->request_failed());
  EXPECT_EQ(net::ERR_FILE_NOT_FOUND, delegate_->request_status());

  ASSERT_FALSE(
      storage::ExternalMountPoints::GetSystemInstance()->RevokeFileSystem(
          kValidExternalMountPoint));
}

}  // namespace
}  // namespace content
