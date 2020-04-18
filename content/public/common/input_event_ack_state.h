// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_INPUT_EVENT_ACK_STATE_H_
#define CONTENT_PUBLIC_COMMON_INPUT_EVENT_ACK_STATE_H_

namespace content {

// Describes the state of the input event ACK coming back to the browser side.
enum InputEventAckState {
  INPUT_EVENT_ACK_STATE_UNKNOWN,
  INPUT_EVENT_ACK_STATE_CONSUMED,
  INPUT_EVENT_ACK_STATE_NOT_CONSUMED,
  INPUT_EVENT_ACK_STATE_CONSUMED_SHOULD_BUBBLE,
  INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS,
  INPUT_EVENT_ACK_STATE_IGNORED,
  INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING,
  INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING_DUE_TO_FLING,
  INPUT_EVENT_ACK_STATE_MAX =
      INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING_DUE_TO_FLING
};

const char* InputEventAckStateToString(InputEventAckState ack_state);

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_INPUT_EVENT_ACK_STATE_H_
