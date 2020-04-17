// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/message_demuxer.h"

#include "platform/test/fake_clock.h"
#include "osp/msgs/osp_messages.h"
#include "osp/public/testing/message_demuxer_test_support.h"
#include "third_party/googletest/src/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"
#include "third_party/tinycbor/src/src/cbor.h"

namespace openscreen {
namespace {

using ::testing::_;
using ::testing::Invoke;

ErrorOr<size_t> ConvertDecodeResult(ssize_t result) {
  if (result < 0) {
    if (result == -CborErrorUnexpectedEOF)
      return Error::Code::kCborIncompleteMessage;
    else
      return Error::Code::kCborParsing;
  } else {
    return result;
  }
}

class MessageDemuxerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(
        msgs::EncodePresentationConnectionOpenRequest(request_, &buffer_));
  }

  void ExpectDecodedRequest(
      ssize_t decode_result,
      const msgs::PresentationConnectionOpenRequest& received_request) {
    ASSERT_GT(decode_result, 0);
    EXPECT_EQ(decode_result, static_cast<ssize_t>(buffer_.size() - 2));
    EXPECT_EQ(request_.request_id, received_request.request_id);
    EXPECT_EQ(request_.presentation_id, received_request.presentation_id);
    EXPECT_EQ(request_.url, received_request.url);
  }

  const uint64_t endpoint_id_ = 13;
  const uint64_t connection_id_ = 45;
  FakeClock fake_clock_{
      platform::Clock::time_point(std::chrono::milliseconds(1298424))};
  msgs::CborEncodeBuffer buffer_;
  msgs::PresentationConnectionOpenRequest request_{1, "fry-am-the-egg-man",
                                                   "url"};
  MockMessageCallback mock_callback_;
  MessageDemuxer demuxer_{FakeClock::now, MessageDemuxer::kDefaultBufferLimit};
};

}  // namespace

TEST_F(MessageDemuxerTest, WatchStartStop) {
  MessageDemuxer::MessageWatch watch = demuxer_.WatchMessageType(
      endpoint_id_, msgs::Type::kPresentationConnectionOpenRequest,
      &mock_callback_);
  ASSERT_TRUE(watch);

  EXPECT_CALL(mock_callback_, OnStreamMessage(_, _, _, _, _, _)).Times(0);
  demuxer_.OnStreamData(endpoint_id_ + 1, 14, buffer_.data(), buffer_.size());

  msgs::PresentationConnectionOpenRequest received_request;
  ssize_t decode_result = 0;
  EXPECT_CALL(
      mock_callback_,
      OnStreamMessage(endpoint_id_, connection_id_,
                      msgs::Type::kPresentationConnectionOpenRequest, _, _, _))
      .WillOnce(
          Invoke([&decode_result, &received_request](
                     uint64_t endpoint_id, uint64_t connection_id,
                     msgs::Type message_type, const uint8_t* buffer,
                     size_t buffer_size, platform::Clock::time_point now) {
            decode_result = msgs::DecodePresentationConnectionOpenRequest(
                buffer, buffer_size, &received_request);
            return ConvertDecodeResult(decode_result);
          }));
  demuxer_.OnStreamData(endpoint_id_, connection_id_, buffer_.data(),
                        buffer_.size());
  ExpectDecodedRequest(decode_result, received_request);

  watch = MessageDemuxer::MessageWatch();
  EXPECT_CALL(mock_callback_, OnStreamMessage(_, _, _, _, _, _)).Times(0);
  demuxer_.OnStreamData(endpoint_id_, connection_id_, buffer_.data(),
                        buffer_.size());
}

TEST_F(MessageDemuxerTest, BufferPartialMessage) {
  MockMessageCallback mock_callback_;
  constexpr uint64_t endpoint_id_ = 13;

  MessageDemuxer::MessageWatch watch = demuxer_.WatchMessageType(
      endpoint_id_, msgs::Type::kPresentationConnectionOpenRequest,
      &mock_callback_);
  ASSERT_TRUE(watch);

  msgs::PresentationConnectionOpenRequest received_request;
  ssize_t decode_result = 0;
  EXPECT_CALL(
      mock_callback_,
      OnStreamMessage(endpoint_id_, connection_id_,
                      msgs::Type::kPresentationConnectionOpenRequest, _, _, _))
      .Times(2)
      .WillRepeatedly(
          Invoke([&decode_result, &received_request](
                     uint64_t endpoint_id, uint64_t connection_id,
                     msgs::Type message_type, const uint8_t* buffer,
                     size_t buffer_size, platform::Clock::time_point now) {
            decode_result = msgs::DecodePresentationConnectionOpenRequest(
                buffer, buffer_size, &received_request);
            return ConvertDecodeResult(decode_result);
          }));
  demuxer_.OnStreamData(endpoint_id_, connection_id_, buffer_.data(),
                        buffer_.size() - 3);
  demuxer_.OnStreamData(endpoint_id_, connection_id_,
                        buffer_.data() + buffer_.size() - 3, 3);
  ExpectDecodedRequest(decode_result, received_request);
}

TEST_F(MessageDemuxerTest, DefaultWatch) {
  MockMessageCallback mock_callback_;
  constexpr uint64_t endpoint_id_ = 13;

  MessageDemuxer::MessageWatch watch = demuxer_.SetDefaultMessageTypeWatch(
      msgs::Type::kPresentationConnectionOpenRequest, &mock_callback_);
  ASSERT_TRUE(watch);

  msgs::PresentationConnectionOpenRequest received_request;
  ssize_t decode_result = 0;
  EXPECT_CALL(
      mock_callback_,
      OnStreamMessage(endpoint_id_, connection_id_,
                      msgs::Type::kPresentationConnectionOpenRequest, _, _, _))
      .WillOnce(
          Invoke([&decode_result, &received_request](
                     uint64_t endpoint_id, uint64_t connection_id,
                     msgs::Type message_type, const uint8_t* buffer,
                     size_t buffer_size, platform::Clock::time_point now) {
            decode_result = msgs::DecodePresentationConnectionOpenRequest(
                buffer, buffer_size, &received_request);
            return ConvertDecodeResult(decode_result);
          }));
  demuxer_.OnStreamData(endpoint_id_, connection_id_, buffer_.data(),
                        buffer_.size());
  ExpectDecodedRequest(decode_result, received_request);
}

TEST_F(MessageDemuxerTest, DefaultWatchOverridden) {
  MockMessageCallback mock_callback_global;
  MockMessageCallback mock_callback_;
  constexpr uint64_t endpoint_id_ = 13;

  MessageDemuxer::MessageWatch default_watch =
      demuxer_.SetDefaultMessageTypeWatch(
          msgs::Type::kPresentationConnectionOpenRequest,
          &mock_callback_global);
  ASSERT_TRUE(default_watch);
  MessageDemuxer::MessageWatch watch = demuxer_.WatchMessageType(
      endpoint_id_, msgs::Type::kPresentationConnectionOpenRequest,
      &mock_callback_);
  ASSERT_TRUE(watch);

  msgs::PresentationConnectionOpenRequest received_request;
  ssize_t decode_result = 0;
  EXPECT_CALL(mock_callback_, OnStreamMessage(_, _, _, _, _, _)).Times(0);
  EXPECT_CALL(
      mock_callback_global,
      OnStreamMessage(endpoint_id_ + 1, 14,
                      msgs::Type::kPresentationConnectionOpenRequest, _, _, _))
      .WillOnce(
          Invoke([&decode_result, &received_request](
                     uint64_t endpoint_id, uint64_t connection_id,
                     msgs::Type message_type, const uint8_t* buffer,
                     size_t buffer_size, platform::Clock::time_point now) {
            decode_result = msgs::DecodePresentationConnectionOpenRequest(
                buffer, buffer_size, &received_request);
            return ConvertDecodeResult(decode_result);
          }));
  demuxer_.OnStreamData(endpoint_id_ + 1, 14, buffer_.data(), buffer_.size());
  ExpectDecodedRequest(decode_result, received_request);

  decode_result = 0;
  EXPECT_CALL(
      mock_callback_,
      OnStreamMessage(endpoint_id_, connection_id_,
                      msgs::Type::kPresentationConnectionOpenRequest, _, _, _))
      .WillOnce(
          Invoke([&decode_result, &received_request](
                     uint64_t endpoint_id, uint64_t connection_id,
                     msgs::Type message_type, const uint8_t* buffer,
                     size_t buffer_size, platform::Clock::time_point now) {
            decode_result = msgs::DecodePresentationConnectionOpenRequest(
                buffer, buffer_size, &received_request);
            return ConvertDecodeResult(decode_result);
          }));
  demuxer_.OnStreamData(endpoint_id_, connection_id_, buffer_.data(),
                        buffer_.size());
  ExpectDecodedRequest(decode_result, received_request);
}

TEST_F(MessageDemuxerTest, WatchAfterData) {
  msgs::PresentationConnectionOpenRequest received_request;
  ssize_t decode_result = 0;
  EXPECT_CALL(
      mock_callback_,
      OnStreamMessage(endpoint_id_, connection_id_,
                      msgs::Type::kPresentationConnectionOpenRequest, _, _, _))
      .WillOnce(
          Invoke([&decode_result, &received_request](
                     uint64_t endpoint_id, uint64_t connection_id,
                     msgs::Type message_type, const uint8_t* buffer,
                     size_t buffer_size, platform::Clock::time_point now) {
            decode_result = msgs::DecodePresentationConnectionOpenRequest(
                buffer, buffer_size, &received_request);
            return ConvertDecodeResult(decode_result);
          }));
  MessageDemuxer::MessageWatch watch = demuxer_.WatchMessageType(
      endpoint_id_, msgs::Type::kPresentationConnectionOpenRequest,
      &mock_callback_);
  ASSERT_TRUE(watch);

  demuxer_.OnStreamData(endpoint_id_, connection_id_, buffer_.data(),
                        buffer_.size());
  ExpectDecodedRequest(decode_result, received_request);
}

TEST_F(MessageDemuxerTest, WatchAfterMultipleData) {
  MockMessageCallback mock_init_callback;
  msgs::PresentationConnectionOpenRequest received_request;
  msgs::PresentationStartRequest received_init_request;
  ssize_t decode_result1 = 0;
  ssize_t decode_result2 = 0;
  MessageDemuxer::MessageWatch init_watch = demuxer_.WatchMessageType(
      endpoint_id_, msgs::Type::kPresentationStartRequest, &mock_init_callback);
  EXPECT_CALL(
      mock_callback_,
      OnStreamMessage(endpoint_id_, connection_id_,
                      msgs::Type::kPresentationConnectionOpenRequest, _, _, _))
      .WillOnce(
          Invoke([&decode_result1, &received_request](
                     uint64_t endpoint_id, uint64_t connection_id,
                     msgs::Type message_type, const uint8_t* buffer,
                     size_t buffer_size, platform::Clock::time_point now) {
            decode_result1 = msgs::DecodePresentationConnectionOpenRequest(
                buffer, buffer_size, &received_request);
            return ConvertDecodeResult(decode_result1);
          }));
  EXPECT_CALL(mock_init_callback,
              OnStreamMessage(endpoint_id_, connection_id_,
                              msgs::Type::kPresentationStartRequest, _, _, _))
      .WillOnce(
          Invoke([&decode_result2, &received_init_request](
                     uint64_t endpoint_id, uint64_t connection_id,
                     msgs::Type message_type, const uint8_t* buffer,
                     size_t buffer_size, platform::Clock::time_point now) {
            decode_result2 = msgs::DecodePresentationStartRequest(
                buffer, buffer_size, &received_init_request);
            return ConvertDecodeResult(decode_result2);
          }));
  MessageDemuxer::MessageWatch watch = demuxer_.WatchMessageType(
      endpoint_id_, msgs::Type::kPresentationConnectionOpenRequest,
      &mock_callback_);
  ASSERT_TRUE(watch);

  demuxer_.OnStreamData(endpoint_id_, connection_id_, buffer_.data(),
                        buffer_.size());

  msgs::CborEncodeBuffer buffer;
  msgs::PresentationStartRequest request;
  request.request_id = 2;
  request.url = "https://example.com/recv";
  ASSERT_TRUE(msgs::EncodePresentationStartRequest(request, &buffer));
  demuxer_.OnStreamData(endpoint_id_, connection_id_, buffer.data(),
                        buffer.size());

  ExpectDecodedRequest(decode_result1, received_request);
  ASSERT_GT(decode_result2, 0);
  EXPECT_EQ(decode_result2, static_cast<ssize_t>(buffer.size() - 2));
  EXPECT_EQ(request.request_id, received_init_request.request_id);
  EXPECT_EQ(request.url, received_init_request.url);
}

TEST_F(MessageDemuxerTest, GlobalWatchAfterData) {
  msgs::PresentationConnectionOpenRequest received_request;
  ssize_t decode_result = 0;
  EXPECT_CALL(
      mock_callback_,
      OnStreamMessage(endpoint_id_, connection_id_,
                      msgs::Type::kPresentationConnectionOpenRequest, _, _, _))
      .WillOnce(
          Invoke([&decode_result, &received_request](
                     uint64_t endpoint_id, uint64_t connection_id,
                     msgs::Type message_type, const uint8_t* buffer,
                     size_t buffer_size, platform::Clock::time_point now) {
            decode_result = msgs::DecodePresentationConnectionOpenRequest(
                buffer, buffer_size, &received_request);
            return ConvertDecodeResult(decode_result);
          }));
  MessageDemuxer::MessageWatch watch = demuxer_.SetDefaultMessageTypeWatch(
      msgs::Type::kPresentationConnectionOpenRequest, &mock_callback_);
  ASSERT_TRUE(watch);
  demuxer_.OnStreamData(endpoint_id_, connection_id_, buffer_.data(),
                        buffer_.size());
  ExpectDecodedRequest(decode_result, received_request);
}

TEST_F(MessageDemuxerTest, BufferLimit) {
  MessageDemuxer demuxer(FakeClock::now, 10);

  demuxer.OnStreamData(endpoint_id_, connection_id_, buffer_.data(),
                       buffer_.size());
  EXPECT_CALL(mock_callback_, OnStreamMessage(_, _, _, _, _, _)).Times(0);
  MessageDemuxer::MessageWatch watch = demuxer.WatchMessageType(
      endpoint_id_, msgs::Type::kPresentationConnectionOpenRequest,
      &mock_callback_);

  msgs::PresentationConnectionOpenRequest received_request;
  ssize_t decode_result = 0;
  EXPECT_CALL(
      mock_callback_,
      OnStreamMessage(endpoint_id_, connection_id_,
                      msgs::Type::kPresentationConnectionOpenRequest, _, _, _))
      .WillOnce(
          Invoke([&decode_result, &received_request](
                     uint64_t endpoint_id, uint64_t connection_id,
                     msgs::Type message_type, const uint8_t* buffer,
                     size_t buffer_size, platform::Clock::time_point now) {
            decode_result = msgs::DecodePresentationConnectionOpenRequest(
                buffer, buffer_size, &received_request);
            return ConvertDecodeResult(decode_result);
          }));
  demuxer.OnStreamData(endpoint_id_, connection_id_, buffer_.data(),
                       buffer_.size());
  ExpectDecodedRequest(decode_result, received_request);
}

TEST_F(MessageDemuxerTest, DeserializeMessages) {
  std::vector<uint8_t> kAgentInfoResponseSerialized{0x0B, 0xFF};
  std::vector<uint8_t> kPresentationConnectionCloseEventSerialized{0x40, 0x71,
                                                                   0x00};
  std::vector<uint8_t> kAuthenticationRequestSerialized{0x43, 0xE9, 0xFF, 0x00};

  size_t used_bytes;
  auto kAgentInfoResponseInfo =
      MessageTypeDecoder::DecodeType(kAgentInfoResponseSerialized, &used_bytes);
  EXPECT_FALSE(kAgentInfoResponseInfo.is_error());
  EXPECT_EQ(used_bytes, size_t{1});
  EXPECT_EQ(kAgentInfoResponseInfo.value(), msgs::Type::kAgentInfoResponse);

  auto kPresentationConnectionCloseEventInfo = MessageTypeDecoder::DecodeType(
      kPresentationConnectionCloseEventSerialized, &used_bytes);
  EXPECT_FALSE(kPresentationConnectionCloseEventInfo.is_error());
  EXPECT_EQ(used_bytes, size_t{2});
  EXPECT_EQ(kPresentationConnectionCloseEventInfo.value(),
            msgs::Type::kPresentationConnectionCloseEvent);

  auto kAuthenticationRequestInfo = MessageTypeDecoder::DecodeType(
      kAuthenticationRequestSerialized, &used_bytes);
  EXPECT_FALSE(kAuthenticationRequestInfo.is_error());
  EXPECT_EQ(used_bytes, size_t{2});
  EXPECT_EQ(kAuthenticationRequestInfo.value(),
            msgs::Type::kAuthenticationRequest);

  auto kUnknownInfo = MessageTypeDecoder::DecodeType({0xFF}, &used_bytes);
  EXPECT_TRUE(kUnknownInfo.is_error());
}

}  // namespace openscreen
