// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/update_client/request_sender.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/update_client/test_configurator.h"
#include "components/update_client/url_request_post_interceptor.h"
#include "net/url_request/url_fetcher.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace update_client {

namespace {

const char kUrl1[] = "https://localhost2/path1";
const char kUrl2[] = "https://localhost2/path2";
const char kUrlPath1[] = "path1";
const char kUrlPath2[] = "path2";

// TODO(sorin): refactor as a utility function for unit tests.
base::FilePath test_file(const char* file) {
  base::FilePath path;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
  return path.AppendASCII("components")
      .AppendASCII("test")
      .AppendASCII("data")
      .AppendASCII("update_client")
      .AppendASCII(file);
}

}  // namespace

class RequestSenderTest : public testing::Test,
                          public ::testing::WithParamInterface<bool> {
 public:
  RequestSenderTest();
  ~RequestSenderTest() override;

  // Overrides from testing::Test.
  void SetUp() override;
  void TearDown() override;

  void RequestSenderComplete(int error,
                             const std::string& response,
                             int retry_after_sec);

 protected:
  void Quit();
  void RunThreads();

  base::test::ScopedTaskEnvironment scoped_task_environment_;

  scoped_refptr<TestConfigurator> config_;
  std::unique_ptr<RequestSender> request_sender_;
  std::unique_ptr<InterceptorFactory> interceptor_factory_;

  scoped_refptr<URLRequestPostInterceptor> post_interceptor_1_;
  scoped_refptr<URLRequestPostInterceptor> post_interceptor_2_;

  int error_ = 0;
  std::string response_;

 private:
  base::OnceClosure quit_closure_;

  DISALLOW_COPY_AND_ASSIGN(RequestSenderTest);
};

INSTANTIATE_TEST_CASE_P(IsForeground, RequestSenderTest, ::testing::Bool());

RequestSenderTest::RequestSenderTest()
    : scoped_task_environment_(
          base::test::ScopedTaskEnvironment::MainThreadType::IO) {}

RequestSenderTest::~RequestSenderTest() {}

void RequestSenderTest::SetUp() {
  config_ = base::MakeRefCounted<TestConfigurator>();
  request_sender_ = std::make_unique<RequestSender>(config_);

  interceptor_factory_ =
      std::make_unique<InterceptorFactory>(base::ThreadTaskRunnerHandle::Get());
  post_interceptor_1_ =
      interceptor_factory_->CreateInterceptorForPath(kUrlPath1);
  post_interceptor_2_ =
      interceptor_factory_->CreateInterceptorForPath(kUrlPath2);
  EXPECT_TRUE(post_interceptor_1_);
  EXPECT_TRUE(post_interceptor_2_);
}

void RequestSenderTest::TearDown() {
  request_sender_ = nullptr;

  post_interceptor_1_ = nullptr;
  post_interceptor_2_ = nullptr;

  interceptor_factory_ = nullptr;

  // Run the threads until they are idle to allow the clean up
  // of the network interceptors on the IO thread.
  scoped_task_environment_.RunUntilIdle();
  config_ = nullptr;
}

void RequestSenderTest::RunThreads() {
  base::RunLoop runloop;
  quit_closure_ = runloop.QuitClosure();
  runloop.Run();
}

void RequestSenderTest::Quit() {
  if (!quit_closure_.is_null())
    std::move(quit_closure_).Run();
}

void RequestSenderTest::RequestSenderComplete(int error,
                                              const std::string& response,
                                              int retry_after_sec) {
  error_ = error;
  response_ = response;

  Quit();
}

// Tests that when a request to the first url succeeds, the subsequent urls are
// not tried.
TEST_P(RequestSenderTest, RequestSendSuccess) {
  EXPECT_TRUE(
      post_interceptor_1_->ExpectRequest(std::make_unique<PartialMatch>("test"),
                                         test_file("updatecheck_reply_1.xml")));

  const bool is_foreground = GetParam();
  request_sender_->Send(
      {GURL(kUrl1), GURL(kUrl2)},
      {{"X-Goog-Update-Interactivity", is_foreground ? "fg" : "bg"}}, "test",
      false,
      base::BindOnce(&RequestSenderTest::RequestSenderComplete,
                     base::Unretained(this)));
  RunThreads();

  EXPECT_EQ(1, post_interceptor_1_->GetHitCount())
      << post_interceptor_1_->GetRequestsAsString();
  EXPECT_EQ(1, post_interceptor_1_->GetCount())
      << post_interceptor_1_->GetRequestsAsString();

  EXPECT_EQ(0, post_interceptor_2_->GetHitCount())
      << post_interceptor_2_->GetRequestsAsString();
  EXPECT_EQ(0, post_interceptor_2_->GetCount())
      << post_interceptor_2_->GetRequestsAsString();

  // Sanity check the request.
  EXPECT_STREQ("test", post_interceptor_1_->GetRequestBody(0).c_str());

  // Check the response post conditions.
  EXPECT_EQ(0, error_);
  EXPECT_TRUE(base::StartsWith(response_,
                               "<?xml version='1.0' encoding='UTF-8'?>",
                               base::CompareCase::SENSITIVE));
  EXPECT_EQ(505ul, response_.size());

  // Check the interactivity header value.
  const auto extra_request_headers =
      post_interceptor_1_->GetRequests()[0].second;
  EXPECT_TRUE(extra_request_headers.HasHeader("X-Goog-Update-Interactivity"));
  std::string header;
  extra_request_headers.GetHeader("X-Goog-Update-Interactivity", &header);
  EXPECT_STREQ(is_foreground ? "fg" : "bg", header.c_str());
}

// Tests that the request succeeds using the second url after the first url
// has failed.
TEST_F(RequestSenderTest, RequestSendSuccessWithFallback) {
  EXPECT_TRUE(post_interceptor_1_->ExpectRequest(
      std::make_unique<PartialMatch>("test"), 403));
  EXPECT_TRUE(post_interceptor_2_->ExpectRequest(
      std::make_unique<PartialMatch>("test")));

  request_sender_->Send(
      {GURL(kUrl1), GURL(kUrl2)}, {}, "test", false,
      base::BindOnce(&RequestSenderTest::RequestSenderComplete,
                     base::Unretained(this)));
  RunThreads();

  EXPECT_EQ(1, post_interceptor_1_->GetHitCount())
      << post_interceptor_1_->GetRequestsAsString();
  EXPECT_EQ(1, post_interceptor_1_->GetCount())
      << post_interceptor_1_->GetRequestsAsString();
  EXPECT_EQ(1, post_interceptor_2_->GetHitCount())
      << post_interceptor_2_->GetRequestsAsString();
  EXPECT_EQ(1, post_interceptor_2_->GetCount())
      << post_interceptor_2_->GetRequestsAsString();

  EXPECT_STREQ("test", post_interceptor_1_->GetRequestBody(0).c_str());
  EXPECT_STREQ("test", post_interceptor_2_->GetRequestBody(0).c_str());
  EXPECT_EQ(0, error_);
}

// Tests that the request fails when both urls have failed.
TEST_F(RequestSenderTest, RequestSendFailed) {
  EXPECT_TRUE(post_interceptor_1_->ExpectRequest(
      std::make_unique<PartialMatch>("test"), 403));
  EXPECT_TRUE(post_interceptor_2_->ExpectRequest(
      std::make_unique<PartialMatch>("test"), 403));

  const std::vector<GURL> urls = {GURL(kUrl1), GURL(kUrl2)};
  request_sender_ = std::make_unique<RequestSender>(config_);
  request_sender_->Send(
      urls, {}, "test", false,
      base::BindOnce(&RequestSenderTest::RequestSenderComplete,
                     base::Unretained(this)));
  RunThreads();

  EXPECT_EQ(1, post_interceptor_1_->GetHitCount())
      << post_interceptor_1_->GetRequestsAsString();
  EXPECT_EQ(1, post_interceptor_1_->GetCount())
      << post_interceptor_1_->GetRequestsAsString();
  EXPECT_EQ(1, post_interceptor_2_->GetHitCount())
      << post_interceptor_2_->GetRequestsAsString();
  EXPECT_EQ(1, post_interceptor_2_->GetCount())
      << post_interceptor_2_->GetRequestsAsString();

  EXPECT_STREQ("test", post_interceptor_1_->GetRequestBody(0).c_str());
  EXPECT_STREQ("test", post_interceptor_2_->GetRequestBody(0).c_str());
  EXPECT_EQ(403, error_);
}

// Tests that the request fails when no urls are provided.
TEST_F(RequestSenderTest, RequestSendFailedNoUrls) {
  std::vector<GURL> urls;
  request_sender_ = std::make_unique<RequestSender>(config_);
  request_sender_->Send(
      urls, std::map<std::string, std::string>(), "test", false,
      base::BindOnce(&RequestSenderTest::RequestSenderComplete,
                     base::Unretained(this)));
  RunThreads();

  EXPECT_EQ(-1, error_);
}

// Tests that a CUP request fails if the response is not signed.
TEST_F(RequestSenderTest, RequestSendCupError) {
  EXPECT_TRUE(
      post_interceptor_1_->ExpectRequest(std::make_unique<PartialMatch>("test"),
                                         test_file("updatecheck_reply_1.xml")));

  const std::vector<GURL> urls = {GURL(kUrl1)};
  request_sender_ = std::make_unique<RequestSender>(config_);
  request_sender_->Send(
      urls, {}, "test", true,
      base::BindOnce(&RequestSenderTest::RequestSenderComplete,
                     base::Unretained(this)));
  RunThreads();

  EXPECT_EQ(1, post_interceptor_1_->GetHitCount())
      << post_interceptor_1_->GetRequestsAsString();
  EXPECT_EQ(1, post_interceptor_1_->GetCount())
      << post_interceptor_1_->GetRequestsAsString();

  EXPECT_STREQ("test", post_interceptor_1_->GetRequestBody(0).c_str());
  EXPECT_EQ(RequestSender::kErrorResponseNotTrusted, error_);
  EXPECT_TRUE(response_.empty());
}

}  // namespace update_client
