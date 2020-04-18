// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/web_rtc_peer_connection_handler_client.h"

namespace blink {

WebRTCPeerConnectionHandlerClient::~WebRTCPeerConnectionHandlerClient() =
    default;

void WebRTCPeerConnectionHandlerClient::ClosePeerConnection() {}

}  // namespace blink
