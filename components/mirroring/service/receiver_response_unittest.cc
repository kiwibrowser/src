// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/mirroring/service/receiver_response.h"

#include "base/base64.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/test/mock_callback.h"
#include "components/mirroring/service/value_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::InvokeWithoutArgs;
using ::testing::_;

namespace mirroring {

class ReceiverResponseTest : public ::testing::Test {
 public:
  ReceiverResponseTest() {}
  ~ReceiverResponseTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ReceiverResponseTest);
};

TEST_F(ReceiverResponseTest, ParseValidJson) {
  const std::string response_string = "{\"type\":\"ANSWER\",\"result\":\"ok\"}";
  ReceiverResponse response;
  ASSERT_TRUE(response.Parse(response_string));
  EXPECT_EQ(-1, response.session_id);
  EXPECT_EQ(-1, response.sequence_number);
  EXPECT_EQ(ResponseType::ANSWER, response.type);
  EXPECT_EQ("ok", response.result);
  EXPECT_FALSE(response.answer);
  EXPECT_FALSE(response.status);
  EXPECT_FALSE(response.capabilities);
  EXPECT_FALSE(response.error);
  EXPECT_TRUE(response.rpc.empty());
}

TEST_F(ReceiverResponseTest, ParseInvalidValueType) {
  const std::string response_string =
      "{\"sessionId\":123, \"seqNum\":\"one-two-three\"}";
  ReceiverResponse response;
  EXPECT_FALSE(response.Parse(response_string));
}

TEST_F(ReceiverResponseTest, ParseNonJsonString) {
  const std::string response_string = "This is not JSON.";
  ReceiverResponse response;
  EXPECT_FALSE(response.Parse(response_string));
}

TEST_F(ReceiverResponseTest, ParseRealWorldAnswerMessage) {
  const std::string response_string =
      "{\"answer\":{\"receiverRtcpEventLog\":[0,1],\"rtpExtensions\":"
      "[[\"adaptive_playout_delay\"],[\"adaptive_playout_delay\"]],"
      "\"sendIndexes\":[0,1],\"ssrcs\":[40863,759293],\"udpPort\":50691,"
      "\"castMode\":\"mirroring\"},\"result\":\"ok\",\"seqNum\":989031000,"
      "\"type\":\"ANSWER\"}";
  ReceiverResponse response;
  ASSERT_TRUE(response.Parse(response_string));
  EXPECT_EQ(-1, response.session_id);
  EXPECT_EQ(989031000, response.sequence_number);
  EXPECT_EQ(ResponseType::ANSWER, response.type);
  EXPECT_EQ("ok", response.result);
  ASSERT_TRUE(response.answer);
  EXPECT_EQ(50691, response.answer->udp_port);
  EXPECT_EQ(std::vector<int32_t>({0, 1}), response.answer->send_indexes);
  EXPECT_EQ(std::vector<int32_t>({40863, 759293}), response.answer->ssrcs);
  EXPECT_TRUE(response.answer->iv.empty());
  EXPECT_EQ(false, response.answer->supports_get_status);
  EXPECT_EQ("mirroring", response.answer->cast_mode);
  EXPECT_FALSE(response.status);
  EXPECT_FALSE(response.capabilities);
  EXPECT_FALSE(response.error);
}

TEST_F(ReceiverResponseTest, ParseErrorMessage) {
  const std::string response_string =
      "{\"sessionId\": 123,"
      "\"seqNum\": 999,"
      "\"type\": \"ANSWER\","
      "\"result\": \"error\","
      "\"error\": {"
      "\"code\": 42,"
      "\"description\": \"it is broke\","
      "\"details\": {\"foo\": -1, \"bar\": 88}"
      "}"
      "}";
  ReceiverResponse response;
  ASSERT_TRUE(response.Parse(response_string));
  EXPECT_EQ(123, response.session_id);
  EXPECT_EQ(999, response.sequence_number);
  EXPECT_EQ(ResponseType::ANSWER, response.type);
  EXPECT_EQ("error", response.result);
  EXPECT_FALSE(response.answer);
  EXPECT_FALSE(response.status);
  EXPECT_FALSE(response.capabilities);
  ASSERT_TRUE(response.error);
  EXPECT_EQ(42, response.error->code);
  EXPECT_EQ("it is broke", response.error->description);
  std::unique_ptr<base::Value> parsed_details =
      base::JSONReader::Read(response.error->details);
  ASSERT_TRUE(parsed_details && parsed_details->is_dict());
  EXPECT_EQ(2u, parsed_details->DictSize());
  int fool_value = 0;
  EXPECT_TRUE(GetInt(*parsed_details, "foo", &fool_value));
  EXPECT_EQ(-1, fool_value);
  int bar_value = 0;
  EXPECT_TRUE(GetInt(*parsed_details, "bar", &bar_value));
  EXPECT_EQ(88, bar_value);
}

TEST_F(ReceiverResponseTest, ParseStatusMessage) {
  const std::string response_string =
      "{\"seqNum\": 777,"
      "\"type\": \"STATUS_RESPONSE\","
      "\"result\": \"ok\","
      "\"status\": {"
      "\"wifiSnr\": 36.7,"
      "\"wifiSpeed\": [1234, 5678, 3000, 3001],"
      "\"wifiFcsError\": [12, 13, 12, 12]}"  // This will be ignored.
      "}";
  ReceiverResponse response;
  ASSERT_TRUE(response.Parse(response_string));
  EXPECT_EQ(777, response.sequence_number);
  EXPECT_EQ(ResponseType::STATUS_RESPONSE, response.type);
  EXPECT_EQ("ok", response.result);
  EXPECT_FALSE(response.error);
  EXPECT_FALSE(response.answer);
  ASSERT_TRUE(response.status);
  EXPECT_EQ(36.7, response.status->wifi_snr);
  const std::vector<int32_t> expect_speed({1234, 5678, 3000, 3001});
  EXPECT_EQ(expect_speed, response.status->wifi_speed);
  EXPECT_FALSE(response.capabilities);
}

TEST_F(ReceiverResponseTest, ParseCapabilityMessage) {
  const std::string response_string =
      "{\"sessionId\": 999888777,"
      "\"seqNum\": 5551212,"
      "\"type\": \"CAPABILITIES_RESPONSE\","
      "\"result\": \"ok\","
      "\"capabilities\": {"
      "\"mediaCaps\": [\"audio\", \"video\", \"vp9\"],"
      "\"keySystems\": ["
      "{"
      "\"keySystemName\": \"com.w3c.clearkey\""
      "},"
      "{"
      "\"keySystemName\": \"com.widevine.alpha\","
      "\"initDataTypes\": [\"a\", \"b\", \"c\", \"1\", \"2\", \"3\"],"
      "\"codecs\": [\"vp8\", \"h264\"],"
      "\"secureCodecs\": [\"h264\", \"vp8\"],"
      "\"audioRobustness\": [\"nope\"],"
      "\"videoRobustness\": [\"yep\"],"
      "\"persistentLicenseSessionSupport\": \"SUPPORTED\","
      "\"persistentReleaseMessageSessionSupport\": \"SUPPORTED_WITH_ID\","
      "\"persistentStateSupport\": \"REQUESTABLE\","
      "\"distinctiveIdentifierSupport\": \"ALWAYS_ENABLED\""
      "}"
      "]}}";
  ReceiverResponse response;
  ASSERT_TRUE(response.Parse(response_string));
  EXPECT_EQ(999888777, response.session_id);
  EXPECT_EQ(5551212, response.sequence_number);
  EXPECT_EQ(ResponseType::CAPABILITIES_RESPONSE, response.type);
  EXPECT_EQ("ok", response.result);
  EXPECT_FALSE(response.error);
  EXPECT_FALSE(response.answer);
  EXPECT_FALSE(response.status);
  ASSERT_TRUE(response.capabilities);
  EXPECT_EQ(std::vector<std::string>({"audio", "video", "vp9"}),
            response.capabilities->media_caps);
  const ReceiverKeySystem& first_key_system =
      response.capabilities->key_systems[0];
  EXPECT_EQ("com.w3c.clearkey", first_key_system.name);
  EXPECT_TRUE(first_key_system.init_data_types.empty());
  EXPECT_TRUE(first_key_system.codecs.empty());
  EXPECT_TRUE(first_key_system.secure_codecs.empty());
  EXPECT_TRUE(first_key_system.audio_robustness.empty());
  EXPECT_TRUE(first_key_system.video_robustness.empty());
  EXPECT_TRUE(first_key_system.persistent_license_session_support.empty());
  EXPECT_TRUE(
      first_key_system.persistent_release_message_session_support.empty());
  EXPECT_TRUE(first_key_system.persistent_state_support.empty());
  EXPECT_TRUE(first_key_system.distinctive_identifier_support.empty());
  const ReceiverKeySystem& second_key_system =
      response.capabilities->key_systems[1];
  EXPECT_EQ("com.widevine.alpha", second_key_system.name);
  EXPECT_EQ(std::vector<std::string>({"a", "b", "c", "1", "2", "3"}),
            second_key_system.init_data_types);
  EXPECT_EQ(std::vector<std::string>({"vp8", "h264"}),
            second_key_system.codecs);
  EXPECT_EQ(std::vector<std::string>({"h264", "vp8"}),
            second_key_system.secure_codecs);
  EXPECT_EQ(std::vector<std::string>({"nope"}),
            second_key_system.audio_robustness);
  EXPECT_EQ(std::vector<std::string>({"yep"}),
            second_key_system.video_robustness);
  EXPECT_EQ("SUPPORTED", second_key_system.persistent_license_session_support);
  EXPECT_EQ("SUPPORTED_WITH_ID",
            second_key_system.persistent_release_message_session_support);
  EXPECT_EQ("REQUESTABLE", second_key_system.persistent_state_support);
  EXPECT_EQ("ALWAYS_ENABLED", second_key_system.distinctive_identifier_support);
}

TEST_F(ReceiverResponseTest, ParseRpcMessage) {
  const std::string message = "Hello from the Cast Receiver!";
  std::string rpc_base64;
  base::Base64Encode(message, &rpc_base64);
  std::string response_string =
      "{\"sessionId\": 735189,"
      "\"seqNum\": 6789,"
      "\"type\": \"RPC\","
      "\"result\": \"ok\","
      "\"rpc\": \"" +
      rpc_base64 + "\"}";
  ReceiverResponse response;
  ASSERT_TRUE(response.Parse(response_string));
  EXPECT_EQ(735189, response.session_id);
  EXPECT_EQ(6789, response.sequence_number);
  EXPECT_EQ("ok", response.result);
  EXPECT_EQ(ResponseType::RPC, response.type);
  EXPECT_EQ(message, response.rpc);
  EXPECT_FALSE(response.error);
  EXPECT_FALSE(response.answer);
  EXPECT_FALSE(response.status);
  EXPECT_FALSE(response.capabilities);
}

TEST_F(ReceiverResponseTest, ParseResponseWithNullField) {
  const std::string response_string =
      "{\"sessionId\":null,\"seqNum\":808907000,\"type\":\"ANSWER\","
      "\"result\":\"ok\",\"rpc\":null,\"error\":null,"
      "\"answer\":{\"udpPort\":51706,\"sendIndexes\":[0,1],"
      "\"ssrcs\":[152818,556029],\"IV\":null,\"receiverGetStatus\":true,"
      "\"castMode\":\"mirroring\"},\"status\":null,\"capabilities\":null}";
  ReceiverResponse response;
  ASSERT_TRUE(response.Parse(response_string));
  EXPECT_EQ(808907000, response.sequence_number);
  EXPECT_EQ("ok", response.result);
  EXPECT_FALSE(response.error);
  EXPECT_FALSE(response.status);
  EXPECT_FALSE(response.capabilities);
  EXPECT_TRUE(response.rpc.empty());
  EXPECT_EQ(ResponseType::ANSWER, response.type);
  ASSERT_TRUE(response.answer);
  EXPECT_EQ(51706, response.answer->udp_port);
  EXPECT_EQ(std::vector<int32_t>({0, 1}), response.answer->send_indexes);
  EXPECT_EQ(std::vector<int32_t>({152818, 556029}), response.answer->ssrcs);
  EXPECT_TRUE(response.answer->iv.empty());
  EXPECT_EQ(true, response.answer->supports_get_status);
  EXPECT_EQ("mirroring", response.answer->cast_mode);
}

}  // namespace mirroring
