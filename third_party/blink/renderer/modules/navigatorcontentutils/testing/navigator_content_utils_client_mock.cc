// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/navigatorcontentutils/testing/navigator_content_utils_client_mock.h"

#include "third_party/blink/renderer/modules/navigatorcontentutils/navigator_content_utils_client.h"
#include "third_party/blink/renderer/platform/wtf/text/string_hash.h"

namespace blink {

void NavigatorContentUtilsClientMock::RegisterProtocolHandler(
    const String& scheme,
    const KURL& url,
    const String& title) {
  ProtocolInfo info;
  info.scheme = scheme;
  info.url = url;
  info.title = title;

  protocol_map_.Set(scheme, info);
}

void NavigatorContentUtilsClientMock::UnregisterProtocolHandler(
    const String& scheme,
    const KURL& url) {
  protocol_map_.erase(scheme);
}

}  // namespace blink
