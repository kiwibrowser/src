// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/url_request_content_job.h"

#include <stdint.h>

#include <algorithm>
#include <memory>

#include "base/files/file_util.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_file_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/resource_request_info.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

// A URLRequestJobFactory that will return URLRequestContentJobWithCallbacks
// instances for content:// scheme URLs.
class CallbacksJobFactory : public net::URLRequestJobFactory {
 public:
  class JobObserver {
   public:
    virtual ~JobObserver() {}
    virtual void OnJobCreated() = 0;
  };

  CallbacksJobFactory(const base::FilePath& path, JobObserver* observer)
      : path_(path), observer_(observer) {
  }

  ~CallbacksJobFactory() override {}

  net::URLRequestJob* MaybeCreateJobWithProtocolHandler(
      const std::string& scheme,
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override {
    URLRequestContentJob* job =
        new URLRequestContentJob(
            request,
            network_delegate,
            path_,
            base::ThreadTaskRunnerHandle::Get());
    observer_->OnJobCreated();
    return job;
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
    return scheme == "content";
  }

  bool IsSafeRedirectTarget(const GURL& location) const override {
    return false;
  }

 private:
  base::FilePath path_;
  JobObserver* observer_;
};

class JobObserverImpl : public CallbacksJobFactory::JobObserver {
 public:
  JobObserverImpl() : num_jobs_created_(0) {}
  ~JobObserverImpl() override {}

  void OnJobCreated() override { ++num_jobs_created_; }

  int num_jobs_created() const { return num_jobs_created_; }

 private:
  int num_jobs_created_;
};

// A simple holder for start/end used in http range requests.
struct Range {
  int start;
  int end;

  explicit Range(int start, int end) {
    this->start = start;
    this->end = end;
  }
};

// A superclass for tests of the OnSeekComplete / OnReadComplete functions of
// URLRequestContentJob.
class URLRequestContentJobTest : public testing::Test {
 public:
  URLRequestContentJobTest();

 protected:
  // This inserts an image file into the android MediaStore and retrieves the
  // content URI. Then creates and runs a URLRequestContentJob to get the
  // contents out of it. If a Range is provided, this function will add the
  // appropriate Range http header to the request and verify the bytes
  // retrieved.
  void RunRequest(const Range* range, const char* intent_type);

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  JobObserverImpl observer_;
  net::TestURLRequestContext context_;
  net::TestDelegate delegate_;
};

URLRequestContentJobTest::URLRequestContentJobTest() {}

void URLRequestContentJobTest::RunRequest(const Range* range,
                                          const char* intent_type) {
  base::FilePath test_dir;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &test_dir);
  test_dir = test_dir.AppendASCII("content");
  test_dir = test_dir.AppendASCII("test");
  test_dir = test_dir.AppendASCII("data");
  test_dir = test_dir.AppendASCII("android");
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
  CallbacksJobFactory factory(path, &observer_);
  context_.set_job_factory(&factory);

  std::unique_ptr<net::URLRequest> request(context_.CreateRequest(
      GURL(path.value()), net::DEFAULT_PRIORITY, &delegate_));

  ResourceRequestInfo::AllocateForTesting(request.get(),
                                          RESOURCE_TYPE_MAIN_FRAME,
                                          nullptr,       // context
                                          0,             // render_process_id
                                          0,             // render_view_id
                                          0,             // render_frame_id
                                          true,          // is_main_frame
                                          false,         // allow_download
                                          true,          // is_async
                                          PREVIEWS_OFF,  // previews_state
                                          nullptr);      // navigation_ui_data

  int expected_length = file_size;
  if (range) {
    ASSERT_GE(range->start, 0);
    ASSERT_GE(range->end, 0);
    ASSERT_LE(range->start, range->end);
    std::string range_value =
        base::StringPrintf("bytes=%d-%d", range->start, range->end);
    request->SetExtraRequestHeaderByName(
        net::HttpRequestHeaders::kRange, range_value, true /*overwrite*/);
    if (range->start <= file_size) {
      expected_length = std::min(range->end, static_cast<int>(file_size - 1)) -
                        range->start + 1;
    } else {
      expected_length = 0;
    }
  }
  std::string expected_mime_type("image/png");
  if (intent_type) {
    request->SetExtraRequestHeaderByName("X-Chrome-intent-type", intent_type,
                                         true /*overwrite*/);
    expected_mime_type = intent_type;
  }
  request->Start();

  base::RunLoop loop;
  loop.Run();

  EXPECT_FALSE(delegate_.request_failed());
  ASSERT_EQ(1, observer_.num_jobs_created());
  EXPECT_EQ(expected_length, delegate_.bytes_received());

  std::string mime_type;
  request->GetMimeType(&mime_type);
  EXPECT_EQ(expected_mime_type, mime_type);
}

// Disabled: http://crbug.com/807045.
TEST_F(URLRequestContentJobTest, DISABLED_ContentURIWithoutRange) {
  RunRequest(NULL, NULL);
}

// Disabled: http://crbug.com/807045.
TEST_F(URLRequestContentJobTest, DISABLED_ContentURIWithSmallRange) {
  Range range(1, 10);
  RunRequest(&range, NULL);
}

// Disabled: http://crbug.com/807045.
TEST_F(URLRequestContentJobTest, DISABLED_ContentURIWithLargeRange) {
  Range range(1, 100000);
  RunRequest(&range, NULL);
}

// Disabled: http://crbug.com/807045.
TEST_F(URLRequestContentJobTest, DISABLED_ContentURIWithZeroRange) {
  Range range(0, 0);
  RunRequest(&range, NULL);
}

TEST_F(URLRequestContentJobTest, ContentURIWithIntentTypeHeader) {
  RunRequest(NULL, "text/html");
}

}  // namespace

}  // namespace content
