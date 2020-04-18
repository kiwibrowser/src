// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/cast_channel/cast_channel_enum_util.h"

namespace extensions {

api::cast_channel::ReadyState ToReadyState(
    ::cast_channel::ReadyState ready_state) {
  switch (ready_state) {
    case ::cast_channel::ReadyState::NONE:
      return api::cast_channel::READY_STATE_NONE;
    case ::cast_channel::ReadyState::CONNECTING:
      return api::cast_channel::READY_STATE_CONNECTING;
    case ::cast_channel::ReadyState::OPEN:
      return api::cast_channel::READY_STATE_OPEN;
    case ::cast_channel::ReadyState::CLOSING:
      return api::cast_channel::READY_STATE_CLOSING;
    case ::cast_channel::ReadyState::CLOSED:
      return api::cast_channel::READY_STATE_CLOSED;
  }
  NOTREACHED() << "Unknown ready_state " << ReadyStateToString(ready_state);
  return api::cast_channel::READY_STATE_NONE;
}

api::cast_channel::ChannelError ToChannelError(
    ::cast_channel::ChannelError channel_error) {
  switch (channel_error) {
    case ::cast_channel::ChannelError::NONE:
      return api::cast_channel::CHANNEL_ERROR_NONE;
    case ::cast_channel::ChannelError::CHANNEL_NOT_OPEN:
      return api::cast_channel::CHANNEL_ERROR_CHANNEL_NOT_OPEN;
    case ::cast_channel::ChannelError::AUTHENTICATION_ERROR:
      return api::cast_channel::CHANNEL_ERROR_AUTHENTICATION_ERROR;
    case ::cast_channel::ChannelError::CONNECT_ERROR:
      return api::cast_channel::CHANNEL_ERROR_CONNECT_ERROR;
    case ::cast_channel::ChannelError::CAST_SOCKET_ERROR:
      return api::cast_channel::CHANNEL_ERROR_SOCKET_ERROR;
    case ::cast_channel::ChannelError::TRANSPORT_ERROR:
      return api::cast_channel::CHANNEL_ERROR_TRANSPORT_ERROR;
    case ::cast_channel::ChannelError::INVALID_MESSAGE:
      return api::cast_channel::CHANNEL_ERROR_INVALID_MESSAGE;
    case ::cast_channel::ChannelError::INVALID_CHANNEL_ID:
      return api::cast_channel::CHANNEL_ERROR_INVALID_CHANNEL_ID;
    case ::cast_channel::ChannelError::CONNECT_TIMEOUT:
      return api::cast_channel::CHANNEL_ERROR_CONNECT_TIMEOUT;
    case ::cast_channel::ChannelError::PING_TIMEOUT:
      return api::cast_channel::CHANNEL_ERROR_PING_TIMEOUT;
    case ::cast_channel::ChannelError::UNKNOWN:
      return api::cast_channel::CHANNEL_ERROR_UNKNOWN;
  }
  NOTREACHED() << "Unknown channel_error "
               << ChannelErrorToString(channel_error);
  return api::cast_channel::CHANNEL_ERROR_NONE;
}

}  // namespace extensions
