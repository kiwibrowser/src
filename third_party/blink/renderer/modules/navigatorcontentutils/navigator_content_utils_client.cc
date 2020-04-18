// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/navigatorcontentutils/navigator_content_utils_client.h"

#include "third_party/blink/public/web/web_frame_client.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {

NavigatorContentUtilsClient* NavigatorContentUtilsClient::Create(
    WebLocalFrameImpl* web_frame) {
  return new NavigatorContentUtilsClient(web_frame);
}

NavigatorContentUtilsClient::NavigatorContentUtilsClient(
    WebLocalFrameImpl* web_frame)
    : web_frame_(web_frame) {}

void NavigatorContentUtilsClient::Trace(blink::Visitor* visitor) {
  visitor->Trace(web_frame_);
}

void NavigatorContentUtilsClient::RegisterProtocolHandler(const String& scheme,
                                                          const KURL& url,
                                                          const String& title) {
  web_frame_->Client()->RegisterProtocolHandler(scheme, url, title);
}

void NavigatorContentUtilsClient::UnregisterProtocolHandler(
    const String& scheme,
    const KURL& url) {
  web_frame_->Client()->UnregisterProtocolHandler(scheme, url);
}

}  // namespace blink
