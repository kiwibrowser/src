// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/renderer/websocket_sb_handshake_throttle.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "content/public/common/resource_type.h"
#include "content/public/renderer/render_frame.h"
#include "ipc/ipc_message.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/http/http_request_headers.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"

namespace safe_browsing {

WebSocketSBHandshakeThrottle::WebSocketSBHandshakeThrottle(
    mojom::SafeBrowsing* safe_browsing,
    int render_frame_id)
    : render_frame_id_(render_frame_id),
      callbacks_(nullptr),
      safe_browsing_(safe_browsing),
      result_(Result::UNKNOWN),
      weak_factory_(this) {}

WebSocketSBHandshakeThrottle::~WebSocketSBHandshakeThrottle() {
  // ThrottleHandshake() should always be called, but since that is done all the
  // way over in Blink, just avoid logging if it is not called rather than
  // DCHECK()ing.
  if (start_time_.is_null())
    return;
  if (result_ == Result::UNKNOWN) {
    result_ = Result::ABANDONED;
    UMA_HISTOGRAM_TIMES("SafeBrowsing.WebSocket.Elapsed.Abandoned",
                        base::TimeTicks::Now() - start_time_);
  }
  UMA_HISTOGRAM_ENUMERATION("SafeBrowsing.WebSocket.Result", result_,
                            Result::RESULT_COUNT);
}

void WebSocketSBHandshakeThrottle::ThrottleHandshake(
    const blink::WebURL& url,
    blink::WebCallbacks<void, const blink::WebString&>* callbacks) {
  DCHECK(!callbacks_);
  DCHECK(!url_checker_);
  callbacks_ = callbacks;
  url_ = url;
  int load_flags = 0;
  start_time_ = base::TimeTicks::Now();
  safe_browsing_->CreateCheckerAndCheck(
      render_frame_id_, mojo::MakeRequest(&url_checker_), url, "GET",
      net::HttpRequestHeaders(), load_flags,
      content::RESOURCE_TYPE_SUB_RESOURCE, false /* has_user_gesture */,
      false /* originated_from_service_worker */,
      base::BindOnce(&WebSocketSBHandshakeThrottle::OnCheckResult,
                     weak_factory_.GetWeakPtr()));

  // This use of base::Unretained() is safe because the handler will not be
  // called after |url_checker_| is destroyed, and it is owned by this object.
  url_checker_.set_connection_error_handler(
      base::BindOnce(&WebSocketSBHandshakeThrottle::OnConnectionError,
                     base::Unretained(this)));
}

void WebSocketSBHandshakeThrottle::OnCompleteCheck(bool proceed,
                                                   bool showed_interstitial) {
  DCHECK(!start_time_.is_null());
  base::TimeDelta elapsed = base::TimeTicks::Now() - start_time_;
  if (proceed) {
    result_ = Result::SAFE;
    UMA_HISTOGRAM_TIMES("SafeBrowsing.WebSocket.Elapsed.Safe", elapsed);
    callbacks_->OnSuccess();
  } else {
    // When the insterstitial is dismissed the page is navigated and this object
    // is destroyed before reaching here.
    result_ = Result::BLOCKED;
    UMA_HISTOGRAM_TIMES("SafeBrowsing.WebSocket.Elapsed.Blocked", elapsed);
    callbacks_->OnError(blink::WebString::FromUTF8(base::StringPrintf(
        "WebSocket connection to %s failed safe browsing check",
        url_.spec().c_str())));
  }
  // |this| is destroyed here.
}

void WebSocketSBHandshakeThrottle::OnCheckResult(
    mojom::UrlCheckNotifierRequest slow_check_notifier,
    bool proceed,
    bool showed_interstitial) {
  if (!slow_check_notifier.is_pending()) {
    OnCompleteCheck(proceed, showed_interstitial);
    return;
  }

  // TODO(yzshen): Notify the network service to pause processing response body.
  if (!notifier_binding_) {
    notifier_binding_ =
        std::make_unique<mojo::Binding<mojom::UrlCheckNotifier>>(this);
  }
  notifier_binding_->Bind(std::move(slow_check_notifier));
}

void WebSocketSBHandshakeThrottle::OnConnectionError() {
  DCHECK_EQ(result_, Result::UNKNOWN);

  url_checker_.reset();
  notifier_binding_.reset();

  // Make the destructor record NOT_SUPPORTED in the result histogram.
  result_ = Result::NOT_SUPPORTED;
  // Don't record the time elapsed because it's unlikely to be meaningful.
  callbacks_->OnSuccess();
  // |this| is destroyed here.
}

}  // namespace safe_browsing
