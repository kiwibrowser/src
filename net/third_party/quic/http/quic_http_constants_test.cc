// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/http/quic_http_constants.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace test {
namespace {

TEST(QuicHttpConstantsTest, QuicHttpFrameType) {
  EXPECT_EQ(QuicHttpFrameType::DATA, static_cast<QuicHttpFrameType>(0));
  EXPECT_EQ(QuicHttpFrameType::HEADERS, static_cast<QuicHttpFrameType>(1));
  EXPECT_EQ(QuicHttpFrameType::QUIC_HTTP_PRIORITY,
            static_cast<QuicHttpFrameType>(2));
  EXPECT_EQ(QuicHttpFrameType::RST_STREAM, static_cast<QuicHttpFrameType>(3));
  EXPECT_EQ(QuicHttpFrameType::SETTINGS, static_cast<QuicHttpFrameType>(4));
  EXPECT_EQ(QuicHttpFrameType::PUSH_PROMISE, static_cast<QuicHttpFrameType>(5));
  EXPECT_EQ(QuicHttpFrameType::PING, static_cast<QuicHttpFrameType>(6));
  EXPECT_EQ(QuicHttpFrameType::GOAWAY, static_cast<QuicHttpFrameType>(7));
  EXPECT_EQ(QuicHttpFrameType::WINDOW_UPDATE,
            static_cast<QuicHttpFrameType>(8));
  EXPECT_EQ(QuicHttpFrameType::CONTINUATION, static_cast<QuicHttpFrameType>(9));
  EXPECT_EQ(QuicHttpFrameType::ALTSVC, static_cast<QuicHttpFrameType>(10));
}

TEST(QuicHttpConstantsTest, QuicHttpFrameTypeToString) {
  EXPECT_EQ("DATA", QuicHttpFrameTypeToString(QuicHttpFrameType::DATA));
  EXPECT_EQ("HEADERS", QuicHttpFrameTypeToString(QuicHttpFrameType::HEADERS));
  EXPECT_EQ("QUIC_HTTP_PRIORITY",
            QuicHttpFrameTypeToString(QuicHttpFrameType::QUIC_HTTP_PRIORITY));
  EXPECT_EQ("RST_STREAM",
            QuicHttpFrameTypeToString(QuicHttpFrameType::RST_STREAM));
  EXPECT_EQ("SETTINGS", QuicHttpFrameTypeToString(QuicHttpFrameType::SETTINGS));
  EXPECT_EQ("PUSH_PROMISE",
            QuicHttpFrameTypeToString(QuicHttpFrameType::PUSH_PROMISE));
  EXPECT_EQ("PING", QuicHttpFrameTypeToString(QuicHttpFrameType::PING));
  EXPECT_EQ("GOAWAY", QuicHttpFrameTypeToString(QuicHttpFrameType::GOAWAY));
  EXPECT_EQ("WINDOW_UPDATE",
            QuicHttpFrameTypeToString(QuicHttpFrameType::WINDOW_UPDATE));
  EXPECT_EQ("CONTINUATION",
            QuicHttpFrameTypeToString(QuicHttpFrameType::CONTINUATION));
  EXPECT_EQ("ALTSVC", QuicHttpFrameTypeToString(QuicHttpFrameType::ALTSVC));

  EXPECT_EQ("DATA", QuicHttpFrameTypeToString(0));
  EXPECT_EQ("HEADERS", QuicHttpFrameTypeToString(1));
  EXPECT_EQ("QUIC_HTTP_PRIORITY", QuicHttpFrameTypeToString(2));
  EXPECT_EQ("RST_STREAM", QuicHttpFrameTypeToString(3));
  EXPECT_EQ("SETTINGS", QuicHttpFrameTypeToString(4));
  EXPECT_EQ("PUSH_PROMISE", QuicHttpFrameTypeToString(5));
  EXPECT_EQ("PING", QuicHttpFrameTypeToString(6));
  EXPECT_EQ("GOAWAY", QuicHttpFrameTypeToString(7));
  EXPECT_EQ("WINDOW_UPDATE", QuicHttpFrameTypeToString(8));
  EXPECT_EQ("CONTINUATION", QuicHttpFrameTypeToString(9));
  EXPECT_EQ("ALTSVC", QuicHttpFrameTypeToString(10));

  EXPECT_EQ("UnknownFrameType(99)", QuicHttpFrameTypeToString(99));
}

TEST(QuicHttpConstantsTest, QuicHttpFrameFlag) {
  EXPECT_EQ(QuicHttpFrameFlag::QUIC_HTTP_END_STREAM,
            static_cast<QuicHttpFrameFlag>(0x01));
  EXPECT_EQ(QuicHttpFrameFlag::QUIC_HTTP_ACK,
            static_cast<QuicHttpFrameFlag>(0x01));
  EXPECT_EQ(QuicHttpFrameFlag::QUIC_HTTP_END_HEADERS,
            static_cast<QuicHttpFrameFlag>(0x04));
  EXPECT_EQ(QuicHttpFrameFlag::QUIC_HTTP_PADDED,
            static_cast<QuicHttpFrameFlag>(0x08));
  EXPECT_EQ(QuicHttpFrameFlag::QUIC_HTTP_PRIORITY,
            static_cast<QuicHttpFrameFlag>(0x20));

  EXPECT_EQ(QuicHttpFrameFlag::QUIC_HTTP_END_STREAM, 0x01);
  EXPECT_EQ(QuicHttpFrameFlag::QUIC_HTTP_ACK, 0x01);
  EXPECT_EQ(QuicHttpFrameFlag::QUIC_HTTP_END_HEADERS, 0x04);
  EXPECT_EQ(QuicHttpFrameFlag::QUIC_HTTP_PADDED, 0x08);
  EXPECT_EQ(QuicHttpFrameFlag::QUIC_HTTP_PRIORITY, 0x20);
}

TEST(QuicHttpConstantsTest, QuicHttpFrameFlagsToString) {
  // Single flags...

  // 0b00000001
  EXPECT_EQ(
      "QUIC_HTTP_END_STREAM",
      QuicHttpFrameFlagsToString(QuicHttpFrameType::DATA,
                                 QuicHttpFrameFlag::QUIC_HTTP_END_STREAM));
  EXPECT_EQ("QUIC_HTTP_END_STREAM",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::HEADERS, 0x01));
  EXPECT_EQ("QUIC_HTTP_ACK",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::SETTINGS,
                                       QuicHttpFrameFlag::QUIC_HTTP_ACK));
  EXPECT_EQ("QUIC_HTTP_ACK",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::PING, 0x01));

  // 0b00000010
  EXPECT_EQ("0x02", QuicHttpFrameFlagsToString(0xff, 0x02));

  // 0b00000100
  EXPECT_EQ(
      "QUIC_HTTP_END_HEADERS",
      QuicHttpFrameFlagsToString(QuicHttpFrameType::HEADERS,
                                 QuicHttpFrameFlag::QUIC_HTTP_END_HEADERS));
  EXPECT_EQ("QUIC_HTTP_END_HEADERS",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::PUSH_PROMISE, 0x04));
  EXPECT_EQ("QUIC_HTTP_END_HEADERS", QuicHttpFrameFlagsToString(0x09, 0x04));
  EXPECT_EQ("0x04", QuicHttpFrameFlagsToString(0xff, 0x04));

  // 0b00001000
  EXPECT_EQ("QUIC_HTTP_PADDED",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::DATA,
                                       QuicHttpFrameFlag::QUIC_HTTP_PADDED));
  EXPECT_EQ("QUIC_HTTP_PADDED",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::HEADERS, 0x08));
  EXPECT_EQ("QUIC_HTTP_PADDED", QuicHttpFrameFlagsToString(0x05, 0x08));
  EXPECT_EQ("0x08", QuicHttpFrameFlagsToString(
                        0xff, QuicHttpFrameFlag::QUIC_HTTP_PADDED));

  // 0b00010000
  EXPECT_EQ("0x10",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::SETTINGS, 0x10));

  // 0b00100000
  EXPECT_EQ("QUIC_HTTP_PRIORITY",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::HEADERS, 0x20));
  EXPECT_EQ("0x20",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::PUSH_PROMISE, 0x20));

  // 0b01000000
  EXPECT_EQ("0x40", QuicHttpFrameFlagsToString(0xff, 0x40));

  // 0b10000000
  EXPECT_EQ("0x80", QuicHttpFrameFlagsToString(0xff, 0x80));

  // Combined flags...

  EXPECT_EQ("QUIC_HTTP_END_STREAM|QUIC_HTTP_PADDED|0xf6",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::DATA, 0xff));
  EXPECT_EQ(
      "QUIC_HTTP_END_STREAM|QUIC_HTTP_END_HEADERS|QUIC_HTTP_PADDED|QUIC_HTTP_"
      "PRIORITY|0xd2",
      QuicHttpFrameFlagsToString(QuicHttpFrameType::HEADERS, 0xff));
  EXPECT_EQ("0xff", QuicHttpFrameFlagsToString(
                        QuicHttpFrameType::QUIC_HTTP_PRIORITY, 0xff));
  EXPECT_EQ("0xff",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::RST_STREAM, 0xff));
  EXPECT_EQ("QUIC_HTTP_ACK|0xfe",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::SETTINGS, 0xff));
  EXPECT_EQ("QUIC_HTTP_END_HEADERS|QUIC_HTTP_PADDED|0xf3",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::PUSH_PROMISE, 0xff));
  EXPECT_EQ("QUIC_HTTP_ACK|0xfe",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::PING, 0xff));
  EXPECT_EQ("0xff",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::GOAWAY, 0xff));
  EXPECT_EQ("0xff",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::WINDOW_UPDATE, 0xff));
  EXPECT_EQ("QUIC_HTTP_END_HEADERS|0xfb",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::CONTINUATION, 0xff));
  EXPECT_EQ("0xff",
            QuicHttpFrameFlagsToString(QuicHttpFrameType::ALTSVC, 0xff));
  EXPECT_EQ("0xff", QuicHttpFrameFlagsToString(0xff, 0xff));
}

TEST(QuicHttpConstantsTest, QuicHttpErrorCode) {
  EXPECT_EQ(QuicHttpErrorCode::HTTP2_NO_ERROR,
            static_cast<QuicHttpErrorCode>(0x0));
  EXPECT_EQ(QuicHttpErrorCode::PROTOCOL_ERROR,
            static_cast<QuicHttpErrorCode>(0x1));
  EXPECT_EQ(QuicHttpErrorCode::INTERNAL_ERROR,
            static_cast<QuicHttpErrorCode>(0x2));
  EXPECT_EQ(QuicHttpErrorCode::FLOW_CONTROL_ERROR,
            static_cast<QuicHttpErrorCode>(0x3));
  EXPECT_EQ(QuicHttpErrorCode::SETTINGS_TIMEOUT,
            static_cast<QuicHttpErrorCode>(0x4));
  EXPECT_EQ(QuicHttpErrorCode::STREAM_CLOSED,
            static_cast<QuicHttpErrorCode>(0x5));
  EXPECT_EQ(QuicHttpErrorCode::FRAME_SIZE_ERROR,
            static_cast<QuicHttpErrorCode>(0x6));
  EXPECT_EQ(QuicHttpErrorCode::REFUSED_STREAM,
            static_cast<QuicHttpErrorCode>(0x7));
  EXPECT_EQ(QuicHttpErrorCode::CANCEL, static_cast<QuicHttpErrorCode>(0x8));
  EXPECT_EQ(QuicHttpErrorCode::COMPRESSION_ERROR,
            static_cast<QuicHttpErrorCode>(0x9));
  EXPECT_EQ(QuicHttpErrorCode::CONNECT_ERROR,
            static_cast<QuicHttpErrorCode>(0xa));
  EXPECT_EQ(QuicHttpErrorCode::ENHANCE_YOUR_CALM,
            static_cast<QuicHttpErrorCode>(0xb));
  EXPECT_EQ(QuicHttpErrorCode::INADEQUATE_SECURITY,
            static_cast<QuicHttpErrorCode>(0xc));
  EXPECT_EQ(QuicHttpErrorCode::HTTP_1_1_REQUIRED,
            static_cast<QuicHttpErrorCode>(0xd));
}

TEST(QuicHttpConstantsTest, QuicHttpErrorCodeToString) {
  EXPECT_EQ("NO_ERROR",
            QuicHttpErrorCodeToString(QuicHttpErrorCode::HTTP2_NO_ERROR));
  EXPECT_EQ("NO_ERROR", QuicHttpErrorCodeToString(0x0));
  EXPECT_EQ("PROTOCOL_ERROR",
            QuicHttpErrorCodeToString(QuicHttpErrorCode::PROTOCOL_ERROR));
  EXPECT_EQ("PROTOCOL_ERROR", QuicHttpErrorCodeToString(0x1));
  EXPECT_EQ("INTERNAL_ERROR",
            QuicHttpErrorCodeToString(QuicHttpErrorCode::INTERNAL_ERROR));
  EXPECT_EQ("INTERNAL_ERROR", QuicHttpErrorCodeToString(0x2));
  EXPECT_EQ("FLOW_CONTROL_ERROR",
            QuicHttpErrorCodeToString(QuicHttpErrorCode::FLOW_CONTROL_ERROR));
  EXPECT_EQ("FLOW_CONTROL_ERROR", QuicHttpErrorCodeToString(0x3));
  EXPECT_EQ("SETTINGS_TIMEOUT",
            QuicHttpErrorCodeToString(QuicHttpErrorCode::SETTINGS_TIMEOUT));
  EXPECT_EQ("SETTINGS_TIMEOUT", QuicHttpErrorCodeToString(0x4));
  EXPECT_EQ("STREAM_CLOSED",
            QuicHttpErrorCodeToString(QuicHttpErrorCode::STREAM_CLOSED));
  EXPECT_EQ("STREAM_CLOSED", QuicHttpErrorCodeToString(0x5));
  EXPECT_EQ("FRAME_SIZE_ERROR",
            QuicHttpErrorCodeToString(QuicHttpErrorCode::FRAME_SIZE_ERROR));
  EXPECT_EQ("FRAME_SIZE_ERROR", QuicHttpErrorCodeToString(0x6));
  EXPECT_EQ("REFUSED_STREAM",
            QuicHttpErrorCodeToString(QuicHttpErrorCode::REFUSED_STREAM));
  EXPECT_EQ("REFUSED_STREAM", QuicHttpErrorCodeToString(0x7));
  EXPECT_EQ("CANCEL", QuicHttpErrorCodeToString(QuicHttpErrorCode::CANCEL));
  EXPECT_EQ("CANCEL", QuicHttpErrorCodeToString(0x8));
  EXPECT_EQ("COMPRESSION_ERROR",
            QuicHttpErrorCodeToString(QuicHttpErrorCode::COMPRESSION_ERROR));
  EXPECT_EQ("COMPRESSION_ERROR", QuicHttpErrorCodeToString(0x9));
  EXPECT_EQ("CONNECT_ERROR",
            QuicHttpErrorCodeToString(QuicHttpErrorCode::CONNECT_ERROR));
  EXPECT_EQ("CONNECT_ERROR", QuicHttpErrorCodeToString(0xa));
  EXPECT_EQ("ENHANCE_YOUR_CALM",
            QuicHttpErrorCodeToString(QuicHttpErrorCode::ENHANCE_YOUR_CALM));
  EXPECT_EQ("ENHANCE_YOUR_CALM", QuicHttpErrorCodeToString(0xb));
  EXPECT_EQ("INADEQUATE_SECURITY",
            QuicHttpErrorCodeToString(QuicHttpErrorCode::INADEQUATE_SECURITY));
  EXPECT_EQ("INADEQUATE_SECURITY", QuicHttpErrorCodeToString(0xc));
  EXPECT_EQ("HTTP_1_1_REQUIRED",
            QuicHttpErrorCodeToString(QuicHttpErrorCode::HTTP_1_1_REQUIRED));
  EXPECT_EQ("HTTP_1_1_REQUIRED", QuicHttpErrorCodeToString(0xd));

  EXPECT_EQ("UnknownErrorCode(0x123)", QuicHttpErrorCodeToString(0x123));
}

TEST(QuicHttpConstantsTest, QuicHttpSettingsParameter) {
  EXPECT_EQ(QuicHttpSettingsParameter::HEADER_TABLE_SIZE,
            static_cast<QuicHttpSettingsParameter>(0x1));
  EXPECT_EQ(QuicHttpSettingsParameter::ENABLE_PUSH,
            static_cast<QuicHttpSettingsParameter>(0x2));
  EXPECT_EQ(QuicHttpSettingsParameter::MAX_CONCURRENT_STREAMS,
            static_cast<QuicHttpSettingsParameter>(0x3));
  EXPECT_EQ(QuicHttpSettingsParameter::INITIAL_WINDOW_SIZE,
            static_cast<QuicHttpSettingsParameter>(0x4));
  EXPECT_EQ(QuicHttpSettingsParameter::MAX_FRAME_SIZE,
            static_cast<QuicHttpSettingsParameter>(0x5));
  EXPECT_EQ(QuicHttpSettingsParameter::MAX_HEADER_LIST_SIZE,
            static_cast<QuicHttpSettingsParameter>(0x6));

  EXPECT_TRUE(IsSupportedQuicHttpSettingsParameter(
      QuicHttpSettingsParameter::HEADER_TABLE_SIZE));
  EXPECT_TRUE(IsSupportedQuicHttpSettingsParameter(
      QuicHttpSettingsParameter::ENABLE_PUSH));
  EXPECT_TRUE(IsSupportedQuicHttpSettingsParameter(
      QuicHttpSettingsParameter::MAX_CONCURRENT_STREAMS));
  EXPECT_TRUE(IsSupportedQuicHttpSettingsParameter(
      QuicHttpSettingsParameter::INITIAL_WINDOW_SIZE));
  EXPECT_TRUE(IsSupportedQuicHttpSettingsParameter(
      QuicHttpSettingsParameter::MAX_FRAME_SIZE));
  EXPECT_TRUE(IsSupportedQuicHttpSettingsParameter(
      QuicHttpSettingsParameter::MAX_HEADER_LIST_SIZE));

  EXPECT_FALSE(IsSupportedQuicHttpSettingsParameter(
      static_cast<QuicHttpSettingsParameter>(0)));
  EXPECT_FALSE(IsSupportedQuicHttpSettingsParameter(
      static_cast<QuicHttpSettingsParameter>(7)));
}

TEST(QuicHttpConstantsTest, QuicHttpSettingsParameterToString) {
  EXPECT_EQ("HEADER_TABLE_SIZE",
            QuicHttpSettingsParameterToString(
                QuicHttpSettingsParameter::HEADER_TABLE_SIZE));
  EXPECT_EQ("HEADER_TABLE_SIZE", QuicHttpSettingsParameterToString(0x1));
  EXPECT_EQ("ENABLE_PUSH", QuicHttpSettingsParameterToString(
                               QuicHttpSettingsParameter::ENABLE_PUSH));
  EXPECT_EQ("ENABLE_PUSH", QuicHttpSettingsParameterToString(0x2));
  EXPECT_EQ("MAX_CONCURRENT_STREAMS",
            QuicHttpSettingsParameterToString(
                QuicHttpSettingsParameter::MAX_CONCURRENT_STREAMS));
  EXPECT_EQ("MAX_CONCURRENT_STREAMS", QuicHttpSettingsParameterToString(0x3));
  EXPECT_EQ("INITIAL_WINDOW_SIZE",
            QuicHttpSettingsParameterToString(
                QuicHttpSettingsParameter::INITIAL_WINDOW_SIZE));
  EXPECT_EQ("INITIAL_WINDOW_SIZE", QuicHttpSettingsParameterToString(0x4));
  EXPECT_EQ("MAX_FRAME_SIZE", QuicHttpSettingsParameterToString(
                                  QuicHttpSettingsParameter::MAX_FRAME_SIZE));
  EXPECT_EQ("MAX_FRAME_SIZE", QuicHttpSettingsParameterToString(0x5));
  EXPECT_EQ("MAX_HEADER_LIST_SIZE",
            QuicHttpSettingsParameterToString(
                QuicHttpSettingsParameter::MAX_HEADER_LIST_SIZE));
  EXPECT_EQ("MAX_HEADER_LIST_SIZE", QuicHttpSettingsParameterToString(0x6));

  EXPECT_EQ("UnknownSettingsParameter(0x123)",
            QuicHttpSettingsParameterToString(0x123));
}

}  // namespace
}  // namespace test
}  // namespace net
