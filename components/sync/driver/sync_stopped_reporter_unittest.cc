// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/sync_stopped_reporter.h"

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/scoped_mock_time_message_loop_task_runner.h"
#include "components/sync/protocol/sync.pb.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

const char kTestURL[] = "http://chromium.org/test";
const char kTestURLTrailingSlash[] = "http://chromium.org/test/";
const char kEventURL[] = "http://chromium.org/test/event";

const char kTestUserAgent[] = "the_fifth_element";
const char kAuthToken[] = "multipass";
const char kCacheGuid[] = "leeloo";
const char kBirthday[] = "2263";

const char kAuthHeaderPrefix[] = "Bearer ";

class SyncStoppedReporterTest : public testing::Test {
 public:
  SyncStoppedReporterTest() {}
  ~SyncStoppedReporterTest() override {}

  void SetUp() override {
    request_context_ =
        new net::TestURLRequestContextGetter(message_loop_.task_runner());
  }

  void RequestFinishedCallback(const SyncStoppedReporter::Result& result) {
    request_result_ = result;
  }

  GURL test_url() { return GURL(kTestURL); }

  std::string user_agent() const { return std::string(kTestUserAgent); }

  SyncStoppedReporter::ResultCallback callback() {
    return base::Bind(&SyncStoppedReporterTest::RequestFinishedCallback,
                      base::Unretained(this));
  }

  const SyncStoppedReporter::Result& request_result() const {
    return request_result_;
  }

  net::URLRequestContextGetter* request_context() {
    return request_context_.get();
  }

 private:
  base::MessageLoop message_loop_;
  scoped_refptr<net::URLRequestContextGetter> request_context_;
  SyncStoppedReporter::Result request_result_;

  DISALLOW_COPY_AND_ASSIGN(SyncStoppedReporterTest);
};

// Test that the event URL gets constructed correctly.
TEST_F(SyncStoppedReporterTest, EventURL) {
  net::TestURLFetcherFactory factory;
  SyncStoppedReporter ssr(GURL(kTestURL), user_agent(), request_context(),
                          callback());
  ssr.ReportSyncStopped(kAuthToken, kCacheGuid, kBirthday);
  net::TestURLFetcher* fetcher = factory.GetFetcherByID(0);
  EXPECT_EQ(kEventURL, fetcher->GetOriginalURL().spec());
}

// Test that the event URL gets constructed correctly with a trailing slash.
TEST_F(SyncStoppedReporterTest, EventURLWithSlash) {
  net::TestURLFetcherFactory factory;
  SyncStoppedReporter ssr(GURL(kTestURLTrailingSlash), user_agent(),
                          request_context(), callback());
  ssr.ReportSyncStopped(kAuthToken, kCacheGuid, kBirthday);
  net::TestURLFetcher* fetcher = factory.GetFetcherByID(0);
  EXPECT_EQ(kEventURL, fetcher->GetOriginalURL().spec());
}

// Test that the URLFetcher gets configured correctly.
TEST_F(SyncStoppedReporterTest, FetcherConfiguration) {
  net::TestURLFetcherFactory factory;
  SyncStoppedReporter ssr(test_url(), user_agent(), request_context(),
                          callback());
  ssr.ReportSyncStopped(kAuthToken, kCacheGuid, kBirthday);
  net::TestURLFetcher* fetcher = factory.GetFetcherByID(0);

  // Ensure the headers are set correctly.
  net::HttpRequestHeaders headers;
  std::string header;
  fetcher->GetExtraRequestHeaders(&headers);
  headers.GetHeader(net::HttpRequestHeaders::kAuthorization, &header);
  std::string auth_header(kAuthHeaderPrefix);
  auth_header.append(kAuthToken);
  EXPECT_EQ(auth_header, header);
  headers.GetHeader(net::HttpRequestHeaders::kUserAgent, &header);
  EXPECT_EQ(user_agent(), header);

  sync_pb::EventRequest event_request;
  event_request.ParseFromString(fetcher->upload_data());

  EXPECT_EQ(kCacheGuid, event_request.sync_disabled().cache_guid());
  EXPECT_EQ(kBirthday, event_request.sync_disabled().store_birthday());
  EXPECT_EQ(kEventURL, fetcher->GetOriginalURL().spec());
}

TEST_F(SyncStoppedReporterTest, HappyCase) {
  net::TestURLFetcherFactory factory;
  SyncStoppedReporter ssr(test_url(), user_agent(), request_context(),
                          callback());
  ssr.ReportSyncStopped(kAuthToken, kCacheGuid, kBirthday);
  net::TestURLFetcher* fetcher = factory.GetFetcherByID(0);
  fetcher->set_response_code(net::HTTP_OK);
  ssr.OnURLFetchComplete(fetcher);
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();
  EXPECT_EQ(SyncStoppedReporter::RESULT_SUCCESS, request_result());
}

TEST_F(SyncStoppedReporterTest, ServerNotFound) {
  net::TestURLFetcherFactory factory;
  SyncStoppedReporter ssr(test_url(), user_agent(), request_context(),
                          callback());
  ssr.ReportSyncStopped(kAuthToken, kCacheGuid, kBirthday);
  net::TestURLFetcher* fetcher = factory.GetFetcherByID(0);
  fetcher->set_response_code(net::HTTP_NOT_FOUND);
  ssr.OnURLFetchComplete(fetcher);
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();
  EXPECT_EQ(SyncStoppedReporter::RESULT_ERROR, request_result());
}

TEST_F(SyncStoppedReporterTest, DestructionDuringRequestHandler) {
  net::TestURLFetcherFactory factory;
  factory.set_remove_fetcher_on_delete(true);
  {
    SyncStoppedReporter ssr(test_url(), user_agent(), request_context(),
                            callback());
    ssr.ReportSyncStopped(kAuthToken, kCacheGuid, kBirthday);
    EXPECT_NE(nullptr, factory.GetFetcherByID(0));
  }
  EXPECT_EQ(nullptr, factory.GetFetcherByID(0));
}

TEST_F(SyncStoppedReporterTest, Timeout) {
  // Mock the underlying loop's clock to trigger the timer at will.
  base::ScopedMockTimeMessageLoopTaskRunner mock_main_runner;

  SyncStoppedReporter ssr(test_url(), user_agent(), request_context(),
                          callback());

  // Begin request.
  ssr.ReportSyncStopped(kAuthToken, kCacheGuid, kBirthday);

  // Trigger the timeout.
  ASSERT_TRUE(mock_main_runner->HasPendingTask());
  mock_main_runner->FastForwardUntilNoTasksRemain();
  EXPECT_EQ(SyncStoppedReporter::RESULT_TIMEOUT, request_result());
}

TEST_F(SyncStoppedReporterTest, NoCallback) {
  net::TestURLFetcherFactory factory;
  SyncStoppedReporter ssr(GURL(kTestURL), user_agent(), request_context(),
                          SyncStoppedReporter::ResultCallback());
  ssr.ReportSyncStopped(kAuthToken, kCacheGuid, kBirthday);
  net::TestURLFetcher* fetcher = factory.GetFetcherByID(0);
  fetcher->set_response_code(net::HTTP_OK);
  ssr.OnURLFetchComplete(fetcher);
}

TEST_F(SyncStoppedReporterTest, NoCallbackTimeout) {
  // Mock the underlying loop's clock to trigger the timer at will.
  base::ScopedMockTimeMessageLoopTaskRunner mock_main_runner;

  SyncStoppedReporter ssr(GURL(kTestURL), user_agent(), request_context(),
                          SyncStoppedReporter::ResultCallback());

  // Begin request.
  ssr.ReportSyncStopped(kAuthToken, kCacheGuid, kBirthday);

  // Trigger the timeout.
  ASSERT_TRUE(mock_main_runner->HasPendingTask());
  mock_main_runner->FastForwardUntilNoTasksRemain();
}

}  // namespace syncer
