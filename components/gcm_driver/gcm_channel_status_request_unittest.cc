// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/gcm_driver/gcm_channel_status_request.h"
#include "components/sync/protocol/experiment_status.pb.h"
#include "components/sync/protocol/experiments_specifics.pb.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gcm {

class GCMChannelStatusRequestTest : public testing::Test {
 public:
  GCMChannelStatusRequestTest();
  ~GCMChannelStatusRequestTest() override;

 protected:
  enum GCMStatus {
    NOT_SPECIFIED,
    GCM_ENABLED,
    GCM_DISABLED,
  };

  void StartRequest();
  void SetResponseStatusAndString(net::HttpStatusCode status_code,
                                  const std::string& response_body);
  void SetResponseProtoData(GCMStatus status, int poll_interval_seconds);
  void CompleteFetch();
  void OnRequestCompleted(bool update_received,
                          bool enabled,
                          int poll_interval_seconds);

  std::unique_ptr<GCMChannelStatusRequest> request_;
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  net::TestURLFetcherFactory url_fetcher_factory_;
  scoped_refptr<net::TestURLRequestContextGetter> url_request_context_getter_;
  bool request_callback_invoked_;
  bool update_received_;
  bool enabled_;
  int poll_interval_seconds_;
};

GCMChannelStatusRequestTest::GCMChannelStatusRequestTest()
    : task_runner_(new base::TestSimpleTaskRunner()),
      task_runner_handle_(task_runner_),
      url_request_context_getter_(
          new net::TestURLRequestContextGetter(task_runner_)),
      request_callback_invoked_(false),
      update_received_(false),
      enabled_(true),
      poll_interval_seconds_(0) {
}

GCMChannelStatusRequestTest::~GCMChannelStatusRequestTest() {
}

void GCMChannelStatusRequestTest::StartRequest() {
  request_.reset(new GCMChannelStatusRequest(
      url_request_context_getter_.get(),
      "http://channel.status.request.com/",
      "user agent string",
      base::Bind(&GCMChannelStatusRequestTest::OnRequestCompleted,
                 base::Unretained(this))));
  request_->Start();
}

void GCMChannelStatusRequestTest::SetResponseStatusAndString(
    net::HttpStatusCode status_code,
    const std::string& response_body) {
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(status_code);
  fetcher->SetResponseString(response_body);
}

void GCMChannelStatusRequestTest::SetResponseProtoData(
    GCMStatus status, int poll_interval_seconds) {
  sync_pb::ExperimentStatusResponse response_proto;
  if (status != NOT_SPECIFIED) {
    sync_pb::ExperimentsSpecifics* experiment_specifics =
        response_proto.add_experiment();
    experiment_specifics->mutable_gcm_channel()->set_enabled(status ==
                                                             GCM_ENABLED);
  }

  // Zero |poll_interval_seconds| means the optional field is not set.
  if (poll_interval_seconds)
    response_proto.set_poll_interval_seconds(poll_interval_seconds);

  std::string response_string;
  response_proto.SerializeToString(&response_string);
  SetResponseStatusAndString(net::HTTP_OK, response_string);
}

void GCMChannelStatusRequestTest::CompleteFetch() {
  request_callback_invoked_ = false;
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
}

void GCMChannelStatusRequestTest::OnRequestCompleted(
    bool update_received, bool enabled, int poll_interval_seconds) {
  request_callback_invoked_ = true;
  update_received_ = update_received;
  enabled_ = enabled;
  poll_interval_seconds_ = poll_interval_seconds;
}

TEST_F(GCMChannelStatusRequestTest, RequestData) {
  StartRequest();

  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  EXPECT_EQ(GURL(request_->channel_status_request_url_),
            fetcher->GetOriginalURL());

  net::HttpRequestHeaders headers;
  fetcher->GetExtraRequestHeaders(&headers);
  std::string user_agent_header;
  headers.GetHeader("User-Agent", &user_agent_header);
  EXPECT_FALSE(user_agent_header.empty());
  EXPECT_EQ(request_->user_agent_, user_agent_header);

  std::string upload_data = fetcher->upload_data();
  EXPECT_FALSE(upload_data.empty());
  sync_pb::ExperimentStatusRequest proto_data;
  proto_data.ParseFromString(upload_data);
  EXPECT_EQ(1, proto_data.experiment_name_size());
  EXPECT_EQ("gcm_channel", proto_data.experiment_name(0));
}

TEST_F(GCMChannelStatusRequestTest, ResponseHttpStatusNotOK) {
  StartRequest();
  SetResponseStatusAndString(net::HTTP_UNAUTHORIZED, "");
  CompleteFetch();

  EXPECT_FALSE(request_callback_invoked_);
}

TEST_F(GCMChannelStatusRequestTest, ResponseEmpty) {
  StartRequest();
  SetResponseStatusAndString(net::HTTP_OK, "");
  CompleteFetch();

  EXPECT_TRUE(request_callback_invoked_);
  EXPECT_FALSE(update_received_);
}

TEST_F(GCMChannelStatusRequestTest, ResponseNotInProtoFormat) {
  StartRequest();
  SetResponseStatusAndString(net::HTTP_OK, "foo");
  CompleteFetch();

  EXPECT_FALSE(request_callback_invoked_);
}

TEST_F(GCMChannelStatusRequestTest, ResponseEmptyProtoData) {
  StartRequest();
  SetResponseProtoData(NOT_SPECIFIED, 0);
  CompleteFetch();

  EXPECT_TRUE(request_callback_invoked_);
  EXPECT_FALSE(update_received_);
}

TEST_F(GCMChannelStatusRequestTest, ResponseWithDisabledStatus) {
  StartRequest();
  SetResponseProtoData(GCM_DISABLED, 0);
  CompleteFetch();

  EXPECT_TRUE(request_callback_invoked_);
  EXPECT_TRUE(update_received_);
  EXPECT_FALSE(enabled_);
  EXPECT_EQ(
      GCMChannelStatusRequest::default_poll_interval_seconds(),
      poll_interval_seconds_);
}

TEST_F(GCMChannelStatusRequestTest, ResponseWithEnabledStatus) {
  StartRequest();
  SetResponseProtoData(GCM_ENABLED, 0);
  CompleteFetch();

  EXPECT_TRUE(request_callback_invoked_);
  EXPECT_TRUE(update_received_);
  EXPECT_TRUE(enabled_);
  EXPECT_EQ(
      GCMChannelStatusRequest::default_poll_interval_seconds(),
      poll_interval_seconds_);
}

TEST_F(GCMChannelStatusRequestTest, ResponseWithPollInterval) {
  // Setting a poll interval 15 minutes longer than the minimum interval we
  // enforce.
  int poll_interval_seconds =
      GCMChannelStatusRequest::min_poll_interval_seconds() + 15 * 60;
  StartRequest();
  SetResponseProtoData(NOT_SPECIFIED, poll_interval_seconds);
  CompleteFetch();

  EXPECT_TRUE(request_callback_invoked_);
  EXPECT_TRUE(update_received_);
  EXPECT_TRUE(enabled_);
  EXPECT_EQ(poll_interval_seconds, poll_interval_seconds_);
}

TEST_F(GCMChannelStatusRequestTest, ResponseWithShortPollInterval) {
  // Setting a poll interval 15 minutes shorter than the minimum interval we
  // enforce.
  int poll_interval_seconds =
      GCMChannelStatusRequest::min_poll_interval_seconds() - 15 * 60;
  StartRequest();
  SetResponseProtoData(NOT_SPECIFIED, poll_interval_seconds);
  CompleteFetch();

  EXPECT_TRUE(request_callback_invoked_);
  EXPECT_TRUE(update_received_);
  EXPECT_TRUE(enabled_);
  EXPECT_EQ(GCMChannelStatusRequest::min_poll_interval_seconds(),
            poll_interval_seconds_);
}

TEST_F(GCMChannelStatusRequestTest, ResponseWithDisabledStatusAndPollInterval) {
  int poll_interval_seconds =
      GCMChannelStatusRequest::min_poll_interval_seconds() + 15 * 60;
  StartRequest();
  SetResponseProtoData(GCM_DISABLED, poll_interval_seconds);
  CompleteFetch();

  EXPECT_TRUE(request_callback_invoked_);
  EXPECT_TRUE(update_received_);
  EXPECT_FALSE(enabled_);
  EXPECT_EQ(poll_interval_seconds, poll_interval_seconds_);
}

}  // namespace gcm
