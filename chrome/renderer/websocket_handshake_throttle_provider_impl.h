// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_WEBSOCKET_HANDSHAKE_THROTTLE_PROVIDER_IMPL_H_
#define CHROME_RENDERER_WEBSOCKET_HANDSHAKE_THROTTLE_PROVIDER_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/safe_browsing/common/safe_browsing.mojom.h"
#include "content/public/renderer/websocket_handshake_throttle_provider.h"

// This must be constructed on the render thread, and then used and destructed
// on a single thread, which can be different from the render thread.
class WebSocketHandshakeThrottleProviderImpl final
    : public content::WebSocketHandshakeThrottleProvider {
 public:
  WebSocketHandshakeThrottleProviderImpl();
  ~WebSocketHandshakeThrottleProviderImpl() override;

  // Implements content::WebSocketHandshakeThrottleProvider.
  std::unique_ptr<content::WebSocketHandshakeThrottleProvider> Clone() override;
  std::unique_ptr<blink::WebSocketHandshakeThrottle> CreateThrottle(
      int render_frame_id) override;

 private:
  // This copy constructor works in conjunction with Clone(), not intended for
  // general use.
  WebSocketHandshakeThrottleProviderImpl(
      const WebSocketHandshakeThrottleProviderImpl& other);

  safe_browsing::mojom::SafeBrowsingPtrInfo safe_browsing_info_;
  safe_browsing::mojom::SafeBrowsingPtr safe_browsing_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_ASSIGN(WebSocketHandshakeThrottleProviderImpl);
};

#endif  // CHROME_RENDERER_WEBSOCKET_HANDSHAKE_THROTTLE_PROVIDER_IMPL_H_
