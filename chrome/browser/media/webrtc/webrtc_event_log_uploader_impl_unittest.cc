// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/webrtc_event_log_uploader.h"

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_status.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::StrictMock;

namespace {
class MockWebRtcEventLogUploaderObserver
    : public WebRtcEventLogUploaderObserver {
 public:
  explicit MockWebRtcEventLogUploaderObserver(
      base::OnceClosure completion_closure)
      : completion_closure_(std::move(completion_closure)) {}

  ~MockWebRtcEventLogUploaderObserver() override = default;

  // Combines the mock functionality via a helper (CompletionCallback), as well
  // as calls the completion closure.
  void OnWebRtcEventLogUploadComplete(const base::FilePath& log_file,
                                      bool upload_successful) override {
    CompletionCallback(log_file, upload_successful);
    std::move(completion_closure_).Run();
  }

  MOCK_METHOD2(CompletionCallback, void(const base::FilePath&, bool));

 private:
  base::OnceClosure completion_closure_;
};

#if defined(OS_POSIX) && !defined(OS_FUCHSIA)
void RemovePermissions(const base::FilePath& path, int removed_permissions) {
  int permissions;
  ASSERT_TRUE(base::GetPosixFilePermissions(path, &permissions));
  permissions &= ~removed_permissions;
  ASSERT_TRUE(base::SetPosixFilePermissions(path, permissions));
}

void RemoveReadPermissions(const base::FilePath& path) {
  constexpr int read_permissions = base::FILE_PERMISSION_READ_BY_USER |
                                   base::FILE_PERMISSION_READ_BY_GROUP |
                                   base::FILE_PERMISSION_READ_BY_OTHERS;
  RemovePermissions(path, read_permissions);
}

void RemoveWritePermissions(const base::FilePath& path) {
  constexpr int write_permissions = base::FILE_PERMISSION_WRITE_BY_USER |
                                    base::FILE_PERMISSION_WRITE_BY_GROUP |
                                    base::FILE_PERMISSION_WRITE_BY_OTHERS;
  RemovePermissions(path, write_permissions);
}
#endif  // defined(OS_POSIX) && !defined(OS_FUCHSIA)
}  // namespace

class WebRtcEventLogUploaderImplTest : public ::testing::Test {
 public:
  WebRtcEventLogUploaderImplTest()
      : observer_run_loop_(),
        url_fetcher_factory_(nullptr),
        observer_(observer_run_loop_.QuitWhenIdleClosure()),
        task_runner_(base::CreateSequencedTaskRunnerWithTraits(
            {base::MayBlock(), base::TaskPriority::BACKGROUND,
             base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {}
  ~WebRtcEventLogUploaderImplTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(log_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(base::CreateTemporaryFileInDir(log_dir_.GetPath(), &log_file_));
    constexpr size_t kLogFileSizeBytes = 100u;
    const std::string file_contents(kLogFileSizeBytes, 'A');
    ASSERT_EQ(
        base::WriteFile(log_file_, file_contents.c_str(), file_contents.size()),
        static_cast<int>(file_contents.size()));
  }

  void TearDown() override {
    base::RunLoop tear_down_run_loop;
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](WebRtcEventLogUploaderImplTest* test,
               base::OnceClosure quit_closure) {
              test->uploader_.reset();
              std::move(quit_closure).Run();
            },
            base::Unretained(this), tear_down_run_loop.QuitWhenIdleClosure()));
    tear_down_run_loop.Run();
  }

  void SetUrlFetcherResponse(net::HttpStatusCode http_code,
                             net::URLRequestStatus::Status request_status) {
    const std::string kResponseId = "ec1ed029734b8f7e";  // Arbitrary.
    url_fetcher_factory_.SetFakeResponse(
        GURL(WebRtcEventLogUploaderImpl::kUploadURL), kResponseId, http_code,
        request_status);
  }

  void StartAndWaitForUpload() {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(
                       [](WebRtcEventLogUploaderImplTest* test) {
                         test->uploader_ = test->uploader_factory_.Create(
                             test->log_file_, &test->observer_);
                       },
                       base::Unretained(this)));
    observer_run_loop_.Run();  // Observer was given quit-closure by ctor.
  }

  void StartAndWaitForUploadWithCustomMaxSize(size_t max_log_size_bytes) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](WebRtcEventLogUploaderImplTest* test,
               size_t max_log_size_bytes) {
              test->uploader_ =
                  test->uploader_factory_.CreateWithCustomMaxSizeForTesting(
                      test->log_file_, &test->observer_, max_log_size_bytes);
            },
            base::Unretained(this), max_log_size_bytes));
    observer_run_loop_.Run();  // Observer was given quit-closure by ctor.
  }

  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  base::RunLoop observer_run_loop_;

  base::ScopedTempDir log_dir_;
  base::FilePath log_file_;

  net::FakeURLFetcherFactory url_fetcher_factory_;

  StrictMock<MockWebRtcEventLogUploaderObserver> observer_;

  // These (uploader-factory and uploader) are the units under test.
  WebRtcEventLogUploaderImpl::Factory uploader_factory_;
  std::unique_ptr<WebRtcEventLogUploader> uploader_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

TEST_F(WebRtcEventLogUploaderImplTest, SuccessfulUploadReportedToObserver) {
  // Main test.
  SetUrlFetcherResponse(net::HTTP_OK, net::URLRequestStatus::SUCCESS);
  EXPECT_CALL(observer_, CompletionCallback(log_file_, true)).Times(1);
  StartAndWaitForUpload();
  EXPECT_FALSE(base::PathExists(log_file_));
}

// Version #1 - request reported as successful, but got an error (404) as the
// HTTP return code.
// Due to the simplicitly of both tests, this also tests the scenario
// FileDeletedAfterUnsuccessfulUpload, rather than giving each its own test.
TEST_F(WebRtcEventLogUploaderImplTest, UnsuccessfulUploadReportedToObserver1) {
  SetUrlFetcherResponse(net::HTTP_NOT_FOUND, net::URLRequestStatus::SUCCESS);
  EXPECT_CALL(observer_, CompletionCallback(log_file_, false)).Times(1);
  StartAndWaitForUpload();
  EXPECT_FALSE(base::PathExists(log_file_));
}

// Version #2 - request reported as failed; HTTP return code ignored, even
// if it's a purported success.
TEST_F(WebRtcEventLogUploaderImplTest, UnsuccessfulUploadReportedToObserver2) {
  SetUrlFetcherResponse(net::HTTP_OK, net::URLRequestStatus::FAILED);
  EXPECT_CALL(observer_, CompletionCallback(log_file_, false)).Times(1);
  StartAndWaitForUpload();
  EXPECT_FALSE(base::PathExists(log_file_));
}

#if defined(OS_POSIX) && !defined(OS_FUCHSIA)
TEST_F(WebRtcEventLogUploaderImplTest, FailureToReadFileReportedToObserver) {
  // Show the failure was independent of the URLFetcher's primed return value.
  SetUrlFetcherResponse(net::HTTP_OK, net::URLRequestStatus::SUCCESS);

  RemoveReadPermissions(log_file_);
  EXPECT_CALL(observer_, CompletionCallback(log_file_, false)).Times(1);
  StartAndWaitForUpload();
}

TEST_F(WebRtcEventLogUploaderImplTest, FailureToDeleteFileHandledGracefully) {
  // Prepare for end of test cleanup.
  int permissions;
  ASSERT_TRUE(base::GetPosixFilePermissions(log_dir_.GetPath(), &permissions));

  // The uploader won't be able to delete the file, but it would be able to
  // read and upload it.
  RemoveWritePermissions(log_dir_.GetPath());
  SetUrlFetcherResponse(net::HTTP_OK, net::URLRequestStatus::SUCCESS);
  EXPECT_CALL(observer_, CompletionCallback(log_file_, true)).Times(1);
  StartAndWaitForUpload();

  // Sanity over the test itself - the file really could not be deleted.
  ASSERT_TRUE(base::PathExists(log_file_));

  // Cleaup
  ASSERT_TRUE(base::SetPosixFilePermissions(log_dir_.GetPath(), permissions));
}
#endif  // defined(OS_POSIX) && !defined(OS_FUCHSIA)

TEST_F(WebRtcEventLogUploaderImplTest, FilesUpToMaxSizeUploaded) {
  int64_t log_file_size_bytes;
  ASSERT_TRUE(base::GetFileSize(log_file_, &log_file_size_bytes));

  SetUrlFetcherResponse(net::HTTP_OK, net::URLRequestStatus::SUCCESS);
  EXPECT_CALL(observer_, CompletionCallback(log_file_, true)).Times(1);
  StartAndWaitForUploadWithCustomMaxSize(log_file_size_bytes);
  EXPECT_FALSE(base::PathExists(log_file_));
}

TEST_F(WebRtcEventLogUploaderImplTest, ExcessivelyLargeFilesNotUploaded) {
  int64_t log_file_size_bytes;
  ASSERT_TRUE(base::GetFileSize(log_file_, &log_file_size_bytes));

  SetUrlFetcherResponse(net::HTTP_OK, net::URLRequestStatus::SUCCESS);
  EXPECT_CALL(observer_, CompletionCallback(log_file_, false)).Times(1);
  StartAndWaitForUploadWithCustomMaxSize(log_file_size_bytes - 1);
  EXPECT_FALSE(base::PathExists(log_file_));
}
