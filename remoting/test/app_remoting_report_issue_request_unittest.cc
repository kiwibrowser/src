// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/app_remoting_report_issue_request.h"

#include "base/bind.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/timer/timer.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const char kTestApplicationId[] = "klasdfjlkasdfjklasjfdkljsadf";
const char kTestHostId[] = "test_host_id_value";
const char kAccessTokenValue[] = "test_access_token_value";
const char kReportIssueResponse[] = "{}";
}  // namespace

namespace remoting {
namespace test {

// Provides base functionality for the AppRemotingReportIssueRequest Tests.  The
// FakeURLFetcherFactory allows us to override the response data and payload for
// specified URLs.  We use this to stub out network calls made by the
// AppRemotingReportIssueRequest.  This fixture also creates an IO MessageLoop,
// if necessary, for use by the AppRemotingReportIssueRequest class.
class AppRemotingReportIssueRequestTest : public ::testing::Test {
 public:
  AppRemotingReportIssueRequestTest();
  ~AppRemotingReportIssueRequestTest() override;

 protected:
  // testing::Test interface.
  void SetUp() override;

  // Sets the HTTP status and data returned for a specified URL.
  void SetFakeResponse(const GURL& url,
                       const std::string& data,
                       net::HttpStatusCode code,
                       net::URLRequestStatus::Status status);

  // Used for result verification.
  std::string dev_service_environment_url_;

  std::unique_ptr<base::RunLoop> run_loop_;
  std::unique_ptr<base::Timer> timer_;

  AppRemotingReportIssueRequest app_remoting_report_issue_request_;

 private:
  net::FakeURLFetcherFactory url_fetcher_factory_;
  std::unique_ptr<base::MessageLoopForIO> message_loop_;

  DISALLOW_COPY_AND_ASSIGN(AppRemotingReportIssueRequestTest);
};

AppRemotingReportIssueRequestTest::AppRemotingReportIssueRequestTest()
    : url_fetcher_factory_(nullptr), message_loop_(new base::MessageLoopForIO) {
}

AppRemotingReportIssueRequestTest::~AppRemotingReportIssueRequestTest() =
    default;

void AppRemotingReportIssueRequestTest::SetUp() {
  run_loop_.reset(new base::RunLoop());
  timer_.reset(new base::Timer(true, false));

  dev_service_environment_url_ =
      GetReportIssueUrl(kTestApplicationId, kTestHostId, kDeveloperEnvironment);
  SetFakeResponse(GURL(dev_service_environment_url_), kReportIssueResponse,
                  net::HTTP_NOT_FOUND, net::URLRequestStatus::FAILED);
}

void AppRemotingReportIssueRequestTest::SetFakeResponse(
    const GURL& url,
    const std::string& data,
    net::HttpStatusCode code,
    net::URLRequestStatus::Status status) {
  url_fetcher_factory_.SetFakeResponse(url, data, code, status);
}

TEST_F(AppRemotingReportIssueRequestTest, ReportIssueFromDev) {
  SetFakeResponse(GURL(dev_service_environment_url_), kReportIssueResponse,
                  net::HTTP_OK, net::URLRequestStatus::SUCCESS);

  timer_->Start(FROM_HERE, base::TimeDelta::FromSeconds(1),
                run_loop_->QuitClosure());

  bool request_started = app_remoting_report_issue_request_.Start(
      kTestApplicationId, kTestHostId, kAccessTokenValue, kDeveloperEnvironment,
      true, run_loop_->QuitClosure());
  EXPECT_TRUE(request_started);

  run_loop_->Run();

  // Verify we stopped because of the request completing and not the timer.
  EXPECT_TRUE(timer_->IsRunning());
  timer_->Stop();
}

TEST_F(AppRemotingReportIssueRequestTest, ReportIssueFromInvalidEnvironment) {
  bool request_started = app_remoting_report_issue_request_.Start(
      kTestApplicationId, kTestHostId, kAccessTokenValue, kUnknownEnvironment,
      true, run_loop_->QuitClosure());

  EXPECT_FALSE(request_started);
}

TEST_F(AppRemotingReportIssueRequestTest, ReportIssueNetworkError) {
  timer_->Start(FROM_HERE, base::TimeDelta::FromSeconds(1),
                run_loop_->QuitClosure());

  bool request_started = app_remoting_report_issue_request_.Start(
      kTestApplicationId, kTestHostId, kAccessTokenValue, kDeveloperEnvironment,
      true, run_loop_->QuitClosure());
  EXPECT_TRUE(request_started);

  run_loop_->Run();

  // Verify we stopped because of the request completing and not the timer.
  EXPECT_TRUE(timer_->IsRunning());
  timer_->Stop();
}

TEST_F(AppRemotingReportIssueRequestTest, MultipleRequestsCanBeIssued) {
  SetFakeResponse(GURL(dev_service_environment_url_), kReportIssueResponse,
                  net::HTTP_OK, net::URLRequestStatus::SUCCESS);

  timer_->Start(FROM_HERE, base::TimeDelta::FromSeconds(1),
                run_loop_->QuitClosure());

  bool request_started = app_remoting_report_issue_request_.Start(
      kTestApplicationId, kTestHostId, kAccessTokenValue, kDeveloperEnvironment,
      true, run_loop_->QuitClosure());
  EXPECT_TRUE(request_started);

  run_loop_->Run();

  // Verify we stopped because of the request completing and not the timer.
  EXPECT_TRUE(timer_->IsRunning());
  timer_->Stop();

  run_loop_.reset(new base::RunLoop());
  timer_->Start(FROM_HERE, base::TimeDelta::FromSeconds(1),
                run_loop_->QuitClosure());

  request_started = app_remoting_report_issue_request_.Start(
      kTestApplicationId, kTestHostId, kAccessTokenValue, kDeveloperEnvironment,
      true, run_loop_->QuitClosure());
  EXPECT_TRUE(request_started);

  run_loop_->Run();

  // Verify we stopped because of the request completing and not the timer.
  EXPECT_TRUE(timer_->IsRunning());
  timer_->Stop();

  run_loop_.reset(new base::RunLoop());
  timer_->Start(FROM_HERE, base::TimeDelta::FromSeconds(1),
                run_loop_->QuitClosure());

  request_started = app_remoting_report_issue_request_.Start(
      kTestApplicationId, kTestHostId, kAccessTokenValue, kDeveloperEnvironment,
      true, run_loop_->QuitClosure());
  EXPECT_TRUE(request_started);

  run_loop_->Run();

  // Verify we stopped because of the request completing and not the timer.
  EXPECT_TRUE(timer_->IsRunning());
  timer_->Stop();
}

}  // namespace test
}  // namespace remoting
