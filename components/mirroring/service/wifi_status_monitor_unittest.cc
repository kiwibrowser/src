// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/mirroring/service/wifi_status_monitor.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "base/values.h"
#include "components/mirroring/service/message_dispatcher.h"
#include "components/mirroring/service/value_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace mirroring {

namespace {

bool IsNullMessage(const CastMessage& message) {
  return message.message_namespace.empty() && message.json_format_data.empty();
}

std::string GetMessageType(const CastMessage& message) {
  std::string type;
  std::unique_ptr<base::Value> value =
      base::JSONReader::Read(message.json_format_data);
  EXPECT_TRUE(value);
  EXPECT_TRUE(GetString(*value, "type", &type));
  return type;
}

void VerifyRecordedStatus(const std::vector<WifiStatus> recorded_status,
                          double starting_snr,
                          int starting_speed,
                          int num_of_responses) {
  EXPECT_EQ(num_of_responses, static_cast<int>(recorded_status.size()));
  for (int i = 0; i < num_of_responses; ++i) {
    EXPECT_EQ(starting_snr + i, recorded_status[i].snr);
    EXPECT_EQ(starting_speed + i, recorded_status[i].speed);
  }
}

}  // namespace

class WifiStatusMonitorTest : public CastMessageChannel,
                              public ::testing::Test {
 public:
  WifiStatusMonitorTest() : message_dispatcher_(this, error_callback_.Get()) {}

  ~WifiStatusMonitorTest() override {}

  // CastMessageChannel implementation. For outbound messages.
  void Send(const CastMessage& message) override {
    last_outbound_message_.message_namespace = message.message_namespace;
    last_outbound_message_.json_format_data = message.json_format_data;
  }

 protected:
  // Generates and sends |num_of_responses| responses.
  void SendStatusResponses(double starting_snr,
                           int starting_speed,
                           int num_of_responses) {
    for (int i = 0; i < num_of_responses; ++i) {
      const std::string response =
          "{\"seqNum\":" +
          std::to_string(message_dispatcher_.GetNextSeqNumber()) +
          ","
          "\"type\": \"STATUS_RESPONSE\","
          "\"result\": \"ok\","
          "\"status\": {"
          "\"wifiSnr\":" +
          std::to_string(starting_snr + i) +
          ","
          "\"wifiSpeed\": [1234, 5678, 3000, " +
          std::to_string(starting_speed + i) +
          "],"
          "\"wifiFcsError\": [12, 13, 12, 12]}"  // This will be ignored.
          "}";
      SendInboundMessage(response);
    }
  }

  // Sends an inbound message to |message_dispatcher|.
  void SendInboundMessage(const std::string& response) {
    CastMessage message;
    message.message_namespace = kWebRtcNamespace;
    message.json_format_data = response;
    static_cast<CastMessageChannel*>(&message_dispatcher_)->Send(message);
    scoped_task_environment_.RunUntilIdle();
  }

  // Creates a WifiStatusMonitor and start monitoring the status.
  std::unique_ptr<WifiStatusMonitor> StartMonitoring() {
    EXPECT_TRUE(IsNullMessage(last_outbound_message_));
    EXPECT_CALL(error_callback_, Run(_)).Times(0);
    auto status_monitor =
        std::make_unique<WifiStatusMonitor>(123, &message_dispatcher_);
    scoped_task_environment_.RunUntilIdle();
    // Expect to receive request to send GET_STATUS message when create a
    // WifiStatusMonitor.
    EXPECT_EQ(kWebRtcNamespace, last_outbound_message_.message_namespace);
    EXPECT_EQ("GET_STATUS", GetMessageType(last_outbound_message_));
    // Clear the old outbound message.
    last_outbound_message_.message_namespace.clear();
    last_outbound_message_.json_format_data.clear();
    EXPECT_TRUE(IsNullMessage(last_outbound_message_));
    return status_monitor;
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::MockCallback<MessageDispatcher::ErrorCallback> error_callback_;
  MessageDispatcher message_dispatcher_;
  CastMessage last_outbound_message_;

 private:
  DISALLOW_COPY_AND_ASSIGN(WifiStatusMonitorTest);
};

TEST_F(WifiStatusMonitorTest, QueryStatusAndRecordResponse) {
  std::unique_ptr<WifiStatusMonitor> status_monitor = StartMonitoring();

  // Send two responses and verify the data are stored.
  SendStatusResponses(36.7, 3001, 2);
  std::vector<WifiStatus> recent_status = status_monitor->GetRecentValues();
  VerifyRecordedStatus(recent_status, 36.7, 3001, 2);

  // There should be no further status stored.
  recent_status = status_monitor->GetRecentValues();
  EXPECT_TRUE(recent_status.empty());

  // Sends more than the maximum number (30) of records that can be stored.
  SendStatusResponses(36.7, 3001, 40);
  // Expect that only the recent 30 records were stored.
  recent_status = status_monitor->GetRecentValues();
  VerifyRecordedStatus(recent_status, 46.7, 3011, 30);
}

TEST_F(WifiStatusMonitorTest, IgnoreMalformedStatusMessage) {
  std::unique_ptr<WifiStatusMonitor> status_monitor = StartMonitoring();

  // Sends a response with incomplete wifiSpeed data and expects it is ignored.
  const std::string response1 =
      "{\"seqNum\": 123,"
      "\"type\": \"STATUS_RESPONSE\","
      "\"result\": \"ok\","
      "\"status\": {"
      "\"wifiSnr\": 32,"
      "\"wifiSpeed\": [1234, 5678, 3000],"
      "\"wifiFcsError\": [12, 13, 12, 12]}"
      "}";
  SendInboundMessage(response1);
  std::vector<WifiStatus> recent_status = status_monitor->GetRecentValues();
  scoped_task_environment_.RunUntilIdle();
  EXPECT_TRUE(recent_status.empty());

  // Sends a response with null status field and expects it is ignored.
  const std::string response2 =
      "{\"seqNum\": 123,"
      "\"type\": \"STATUS_RESPONSE\","
      "\"status\": null}";
  SendInboundMessage(response2);
  recent_status = status_monitor->GetRecentValues();
  scoped_task_environment_.RunUntilIdle();
  EXPECT_TRUE(recent_status.empty());
}

}  // namespace mirroring
