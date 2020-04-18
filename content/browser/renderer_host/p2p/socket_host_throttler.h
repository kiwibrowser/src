// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_HOST_THROTTLER_H_
#define CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_HOST_THROTTLER_H_

#include <stddef.h>

#include <memory>

#include "base/macros.h"
#include "content/common/content_export.h"

namespace rtc {
class DataRateLimiter;
}

namespace content {

// A very simple message throtller. User of this class must drop the packet if
// DropNextPacket returns false for that packet. This method verifies the
// current sendrate against the required sendrate.

class CONTENT_EXPORT P2PMessageThrottler {
 public:
  P2PMessageThrottler();
  virtual ~P2PMessageThrottler();

  bool DropNextPacket(size_t packet_len);
  void SetSendIceBandwidth(int bandwith_kbps);

 private:
  std::unique_ptr<rtc::DataRateLimiter> rate_limiter_;

  DISALLOW_COPY_AND_ASSIGN(P2PMessageThrottler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_HOST_THROTTLER_H_
