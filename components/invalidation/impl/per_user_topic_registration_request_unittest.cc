// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/per_user_topic_registration_request.h"

#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"

#include "base/strings/stringprintf.h"
#include "base/test/gtest_util.h"
#include "base/test/mock_callback.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "components/invalidation/impl/json_unsafe_parser.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

using testing::_;
using testing::SaveArg;

MATCHER_P(EqualsJSON, json, "equals JSON") {
  std::unique_ptr<base::Value> expected = base::JSONReader::Read(json);
  if (!expected) {
    *result_listener << "INTERNAL ERROR: couldn't parse expected JSON";
    return false;
  }

  std::string err_msg;
  int err_line, err_col;
  std::unique_ptr<base::Value> actual = base::JSONReader::ReadAndReturnError(
      arg, base::JSON_PARSE_RFC, nullptr, &err_msg, &err_line, &err_col);
  if (!actual) {
    *result_listener << "input:" << err_line << ":" << err_col << ": "
                     << "parse error: " << err_msg;
    return false;
  }
  return *expected == *actual;
}

network::ResourceResponseHead CreateHeadersForTest(int responce_code) {
  network::ResourceResponseHead head;
  head.headers = new net::HttpResponseHeaders(base::StringPrintf(
      "HTTP/1.1 %d OK\nContent-type: text/html\n\n", responce_code));
  head.mime_type = "text/html";
  return head;
}

}  // namespace

class PerUserTopicRegistrationRequestTest : public testing::Test {
 public:
  PerUserTopicRegistrationRequestTest() {}

  GURL url(PerUserTopicRegistrationRequest* request) {
    return request->getUrl();
  }

  network::TestURLLoaderFactory* url_loader_factory() {
    return &url_loader_factory_;
  }

 private:
  base::MessageLoop message_loop_;
  network::TestURLLoaderFactory url_loader_factory_;

  DISALLOW_COPY_AND_ASSIGN(PerUserTopicRegistrationRequestTest);
};

TEST_F(PerUserTopicRegistrationRequestTest,
       ShouldNotInvokeCallbackWhenCancelled) {
  std::string token = "1234567890";
  std::string url = "http://valid-url.test";
  std::string topic = "test";
  std::string project_id = "smarty-pants-12345";

  base::MockCallback<PerUserTopicRegistrationRequest::CompletedCallback>
      callback;
  EXPECT_CALL(callback, Run(_, _)).Times(0);

  PerUserTopicRegistrationRequest::Builder builder;
  std::unique_ptr<PerUserTopicRegistrationRequest> request =
      builder.SetToken(token)
          .SetScope(url)
          .SetPublicTopicName(topic)
          .SetProjectId(project_id)
          .Build();
  request->Start(callback.Get(),
                 base::BindOnce(syncer::JsonUnsafeParser::Parse),
                 url_loader_factory());
  base::RunLoop().RunUntilIdle();

  // Destroy the request before getting any response.
  request.reset();
}

TEST_F(PerUserTopicRegistrationRequestTest, ShouldSubscribeWithoutErrors) {
  std::string token = "1234567890";
  std::string base_url = "http://valid-url.test";
  std::string topic = "test";
  std::string project_id = "smarty-pants-12345";

  base::MockCallback<PerUserTopicRegistrationRequest::CompletedCallback>
      callback;
  Status status(StatusCode::FAILED, "initial");
  std::string private_topic;
  EXPECT_CALL(callback, Run(_, _))
      .WillOnce(DoAll(SaveArg<0>(&status), SaveArg<1>(&private_topic)));

  PerUserTopicRegistrationRequest::Builder builder;
  std::unique_ptr<PerUserTopicRegistrationRequest> request =
      builder.SetToken(token)
          .SetScope(base_url)
          .SetPublicTopicName(topic)
          .SetProjectId(project_id)
          .Build();
  std::string response_body = R"(
    {
      "private_topic_name": "test-pr"
    }
  )";

  network::URLLoaderCompletionStatus response_status(net::OK);
  response_status.decoded_body_length = response_body.size();

  url_loader_factory()->AddResponse(url(request.get()),
                                    CreateHeadersForTest(net::HTTP_OK),
                                    response_body, response_status);
  request->Start(callback.Get(),
                 base::BindOnce(syncer::JsonUnsafeParser::Parse),
                 url_loader_factory());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(status.code, StatusCode::SUCCESS);
  EXPECT_EQ(private_topic, "test-pr");
}

TEST_F(PerUserTopicRegistrationRequestTest,
       ShouleNotSubscribeWhenNetworkProblem) {
  std::string token = "1234567890";
  std::string base_url = "http://valid-url.test";
  std::string topic = "test";
  std::string project_id = "smarty-pants-12345";

  base::MockCallback<PerUserTopicRegistrationRequest::CompletedCallback>
      callback;
  Status status(StatusCode::FAILED, "initial");
  std::string private_topic;
  EXPECT_CALL(callback, Run(_, _))
      .WillOnce(DoAll(SaveArg<0>(&status), SaveArg<1>(&private_topic)));

  PerUserTopicRegistrationRequest::Builder builder;
  std::unique_ptr<PerUserTopicRegistrationRequest> request =
      builder.SetToken(token)
          .SetScope(base_url)
          .SetPublicTopicName(topic)
          .SetProjectId(project_id)
          .Build();
  std::string response_body = R"(
    {
      "private_topic_name": "test-pr"
    }
  )";

  network::URLLoaderCompletionStatus response_status(net::ERR_TIMED_OUT);
  response_status.decoded_body_length = response_body.size();

  url_loader_factory()->AddResponse(url(request.get()),
                                    CreateHeadersForTest(net::HTTP_OK),
                                    response_body, response_status);
  request->Start(callback.Get(),
                 base::BindOnce(syncer::JsonUnsafeParser::Parse),
                 url_loader_factory());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(status.code, StatusCode::FAILED);
}

TEST_F(PerUserTopicRegistrationRequestTest,
       ShouldNotSubscribeWhenWrongResponse) {
  std::string token = "1234567890";
  std::string base_url = "http://valid-url.test";
  std::string topic = "test";
  std::string project_id = "smarty-pants-12345";

  base::MockCallback<PerUserTopicRegistrationRequest::CompletedCallback>
      callback;
  Status status(StatusCode::FAILED, "initial");
  std::string private_topic;

  EXPECT_CALL(callback, Run(_, _))
      .WillOnce(DoAll(SaveArg<0>(&status), SaveArg<1>(&private_topic)));

  PerUserTopicRegistrationRequest::Builder builder;
  std::unique_ptr<PerUserTopicRegistrationRequest> request =
      builder.SetToken(token)
          .SetScope(base_url)
          .SetPublicTopicName(topic)
          .SetProjectId(project_id)
          .Build();
  std::string response_body = R"(
    {}
  )";

  network::URLLoaderCompletionStatus response_status(net::OK);
  response_status.decoded_body_length = response_body.size();

  url_loader_factory()->AddResponse(url(request.get()),
                                    CreateHeadersForTest(net::HTTP_OK),
                                    response_body, response_status);
  request->Start(callback.Get(),
                 base::BindOnce(syncer::JsonUnsafeParser::Parse),
                 url_loader_factory());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(status.code, StatusCode::FAILED);
}

}  // namespace syncer
