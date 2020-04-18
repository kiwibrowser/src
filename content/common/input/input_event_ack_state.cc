// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/input_event_ack_state.h"

#include "base/logging.h"

namespace content {

const char* InputEventAckStateToString(InputEventAckState ack_state) {
  switch (ack_state) {
    case INPUT_EVENT_ACK_STATE_UNKNOWN:
      return "UNKNOWN";
    case INPUT_EVENT_ACK_STATE_CONSUMED:
      return "CONSUMED";
    case INPUT_EVENT_ACK_STATE_NOT_CONSUMED:
      return "NOT_CONSUMED";
    case INPUT_EVENT_ACK_STATE_CONSUMED_SHOULD_BUBBLE:
      return "CONSUMED_SHOULD_BUBBLE";
    case INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS:
      return "NO_CONSUMER_EXISTS";
    case INPUT_EVENT_ACK_STATE_IGNORED:
      return "IGNORED";
    case INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING:
      return "SET_NON_BLOCKING";
    case INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING_DUE_TO_FLING:
      return "SET_NON_BLOCKING_DUE_TO_FLING";
  }
  DLOG(WARNING) << "InputEventAckStateToString: Unhandled InputEventAckState.";
  return "";
}

}  // namespace content
