// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_WEBSOCKET_HANDSHAKE_THROTTLE_PROVIDER_H_
#define CONTENT_PUBLIC_RENDERER_WEBSOCKET_HANDSHAKE_THROTTLE_PROVIDER_H_

#include <memory>

#include "content/common/content_export.h"

namespace blink {
class WebSocketHandshakeThrottle;
}

namespace content {

// This interface allows the embedder to provide a WebSocketHandshakeThrottle
// implementation. An instance of this class must be constructed on the render
// thread, and then used and destructed on a single thread, which can be
// different from the render thread.
class CONTENT_EXPORT WebSocketHandshakeThrottleProvider {
 public:
  virtual ~WebSocketHandshakeThrottleProvider() {}

  // Used to copy a WebSocketHandshakeThrottleProvider between worker threads.
  virtual std::unique_ptr<WebSocketHandshakeThrottleProvider> Clone() = 0;

  // For requests from frames and dedicated workers, |render_frame_id| should be
  // set to the corresponding frame. For requests from shared or service
  // workers, |render_frame_id| should be set to MSG_ROUTING_NONE.
  virtual std::unique_ptr<blink::WebSocketHandshakeThrottle> CreateThrottle(
      int render_frame_id) = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_WEBSOCKET_HANDSHAKE_THROTTLE_PROVIDER_H_
