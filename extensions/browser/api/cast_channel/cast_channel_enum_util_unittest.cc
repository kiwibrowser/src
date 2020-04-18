// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/cast_channel/cast_channel_enum_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

TEST(CastChannelEnumUtilTest, TestToReadyState) {
  EXPECT_EQ(api::cast_channel::READY_STATE_NONE,
            ToReadyState(::cast_channel::ReadyState::NONE));
  EXPECT_EQ(api::cast_channel::READY_STATE_CONNECTING,
            ToReadyState(::cast_channel::ReadyState::CONNECTING));
  EXPECT_EQ(api::cast_channel::READY_STATE_OPEN,
            ToReadyState(::cast_channel::ReadyState::OPEN));
  EXPECT_EQ(api::cast_channel::READY_STATE_CLOSING,
            ToReadyState(::cast_channel::ReadyState::CLOSING));
  EXPECT_EQ(api::cast_channel::READY_STATE_CLOSED,
            ToReadyState(::cast_channel::ReadyState::CLOSED));
}

TEST(CastChannelEnumUtilTest, TestToChannelError) {
  EXPECT_EQ(api::cast_channel::CHANNEL_ERROR_NONE,
            ToChannelError(::cast_channel::ChannelError::NONE));
  EXPECT_EQ(api::cast_channel::CHANNEL_ERROR_CHANNEL_NOT_OPEN,
            ToChannelError(::cast_channel::ChannelError::CHANNEL_NOT_OPEN));
  EXPECT_EQ(api::cast_channel::CHANNEL_ERROR_AUTHENTICATION_ERROR,
            ToChannelError(::cast_channel::ChannelError::AUTHENTICATION_ERROR));
  EXPECT_EQ(api::cast_channel::CHANNEL_ERROR_CONNECT_ERROR,
            ToChannelError(::cast_channel::ChannelError::CONNECT_ERROR));
  EXPECT_EQ(api::cast_channel::CHANNEL_ERROR_SOCKET_ERROR,
            ToChannelError(::cast_channel::ChannelError::CAST_SOCKET_ERROR));
  EXPECT_EQ(api::cast_channel::CHANNEL_ERROR_TRANSPORT_ERROR,
            ToChannelError(::cast_channel::ChannelError::TRANSPORT_ERROR));
  EXPECT_EQ(api::cast_channel::CHANNEL_ERROR_INVALID_MESSAGE,
            ToChannelError(::cast_channel::ChannelError::INVALID_MESSAGE));
  EXPECT_EQ(api::cast_channel::CHANNEL_ERROR_INVALID_CHANNEL_ID,
            ToChannelError(::cast_channel::ChannelError::INVALID_CHANNEL_ID));
  EXPECT_EQ(api::cast_channel::CHANNEL_ERROR_CONNECT_TIMEOUT,
            ToChannelError(::cast_channel::ChannelError::CONNECT_TIMEOUT));
  EXPECT_EQ(api::cast_channel::CHANNEL_ERROR_PING_TIMEOUT,
            ToChannelError(::cast_channel::ChannelError::PING_TIMEOUT));
  EXPECT_EQ(api::cast_channel::CHANNEL_ERROR_UNKNOWN,
            ToChannelError(::cast_channel::ChannelError::UNKNOWN));
}

}  // namespace extensions
