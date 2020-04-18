// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_CAPABILITY_NAMES_H_
#define REMOTING_PROTOCOL_CAPABILITY_NAMES_H_

namespace remoting {
namespace protocol {

// Used for negotiating client-host capabilities for touch events.
const char kTouchEventsCapability[] = "touchEvents";

const char kSendInitialResolution[] = "sendInitialResolution";
const char kRateLimitResizeRequests[] = "rateLimitResizeRequests";

const char kFileTransferCapability[] = "fileTransfer";

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_CAPABILITY_NAMES_H_
