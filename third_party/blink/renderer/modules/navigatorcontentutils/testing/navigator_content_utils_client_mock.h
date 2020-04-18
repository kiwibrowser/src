// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_NAVIGATORCONTENTUTILS_TESTING_NAVIGATOR_CONTENT_UTILS_CLIENT_MOCK_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_NAVIGATORCONTENTUTILS_TESTING_NAVIGATOR_CONTENT_UTILS_CLIENT_MOCK_H_

#include "third_party/blink/renderer/modules/navigatorcontentutils/navigator_content_utils_client.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// Provides a mock object for the navigatorcontentutils client.
class NavigatorContentUtilsClientMock final
    : public NavigatorContentUtilsClient {
 public:
  static NavigatorContentUtilsClientMock* Create() {
    return new NavigatorContentUtilsClientMock;
  }

  ~NavigatorContentUtilsClientMock() override = default;

  void RegisterProtocolHandler(const String& scheme,
                               const KURL&,
                               const String& title) override;

  void UnregisterProtocolHandler(const String& scheme, const KURL&) override;

 private:
  // TODO(sashab): Make NavigatorContentUtilsClientMock non-virtual and test it
  // using a WebFrameClient mock.
  NavigatorContentUtilsClientMock() : NavigatorContentUtilsClient(nullptr) {}

  typedef struct {
    String scheme;
    KURL url;
    String title;
  } ProtocolInfo;

  typedef HashMap<String, ProtocolInfo> RegisteredProtocolMap;
  RegisteredProtocolMap protocol_map_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_NAVIGATORCONTENTUTILS_TESTING_NAVIGATOR_CONTENT_UTILS_CLIENT_MOCK_H_
