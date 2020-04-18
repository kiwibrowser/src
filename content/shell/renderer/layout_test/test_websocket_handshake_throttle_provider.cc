// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/renderer/layout_test/test_websocket_handshake_throttle_provider.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "third_party/blink/public/platform/web_callbacks.h"
#include "third_party/blink/public/platform/web_url.h"
#include "url/gurl.h"

namespace content {

namespace {

using Callbacks = blink::WebCallbacks<void, const blink::WebString&>;

// Checks for a valid "content-shell-websocket-delay-ms" parameter and returns
// it as a TimeDelta if it exists. Otherwise returns a zero TimeDelta.
base::TimeDelta ExtractDelayFromUrl(const GURL& url) {
  if (!url.has_query())
    return base::TimeDelta();
  url::Component query = url.parsed_for_possibly_invalid_spec().query;
  url::Component key;
  url::Component value;
  base::StringPiece spec = url.possibly_invalid_spec();
  while (url::ExtractQueryKeyValue(spec.data(), &query, &key, &value)) {
    base::StringPiece key_piece = spec.substr(key.begin, key.len);
    if (key_piece != "content-shell-websocket-delay-ms")
      continue;

    base::StringPiece value_piece = spec.substr(value.begin, value.len);
    int value_int;
    if (!base::StringToInt(value_piece, &value_int) || value_int < 0)
      return base::TimeDelta();

    return base::TimeDelta::FromMilliseconds(value_int);
  }

  // Parameter was not found.
  return base::TimeDelta();
}

// A simple WebSocketHandshakeThrottle that calls callbacks->IsSuccess() after n
// milli-seconds if the URL query contains
// content-shell-websocket-delay-ms=n. Otherwise it calls IsSuccess()
// immediately.
class TestWebSocketHandshakeThrottle
    : public blink::WebSocketHandshakeThrottle {
 public:
  ~TestWebSocketHandshakeThrottle() override = default;

  void ThrottleHandshake(const blink::WebURL& url,
                         Callbacks* callbacks) override {
    DCHECK(callbacks);

    // This use of Unretained is safe because this object is destroyed before
    // |callbacks| is freed. Destroying this object prevents the timer from
    // firing.
    timer_.Start(FROM_HERE, ExtractDelayFromUrl(url),
                 base::BindRepeating(&Callbacks::OnSuccess,
                                     base::Unretained(callbacks)));
  }

 private:
  base::OneShotTimer timer_;
};

}  // namespace

std::unique_ptr<content::WebSocketHandshakeThrottleProvider>
TestWebSocketHandshakeThrottleProvider::Clone() {
  return std::make_unique<TestWebSocketHandshakeThrottleProvider>();
}

std::unique_ptr<blink::WebSocketHandshakeThrottle>
TestWebSocketHandshakeThrottleProvider::CreateThrottle(int render_frame_id) {
  return std::make_unique<TestWebSocketHandshakeThrottle>();
}

}  // namespace content
