// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/rtcp_session.h"

#include "platform/api/logging.h"

namespace openscreen {
namespace cast_streaming {

RtcpSession::RtcpSession(Ssrc sender_ssrc, Ssrc receiver_ssrc)
    : sender_ssrc_(sender_ssrc), receiver_ssrc_(receiver_ssrc) {
  OSP_DCHECK_NE(sender_ssrc_, receiver_ssrc_);
}

RtcpSession::~RtcpSession() = default;

}  // namespace cast_streaming
}  // namespace openscreen
