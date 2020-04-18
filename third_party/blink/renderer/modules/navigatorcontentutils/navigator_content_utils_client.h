// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_NAVIGATORCONTENTUTILS_NAVIGATOR_CONTENT_UTILS_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_NAVIGATORCONTENTUTILS_NAVIGATOR_CONTENT_UTILS_CLIENT_H_

#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class KURL;
class WebLocalFrameImpl;

class MODULES_EXPORT NavigatorContentUtilsClient
    : public GarbageCollectedFinalized<NavigatorContentUtilsClient> {
 public:
  static NavigatorContentUtilsClient* Create(WebLocalFrameImpl*);
  virtual ~NavigatorContentUtilsClient() = default;

  virtual void RegisterProtocolHandler(const String& scheme,
                                       const KURL&,
                                       const String& title);

  virtual void UnregisterProtocolHandler(const String& scheme, const KURL&);

  virtual void Trace(blink::Visitor*);

 protected:
  explicit NavigatorContentUtilsClient(WebLocalFrameImpl*);

 private:
  Member<WebLocalFrameImpl> web_frame_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_NAVIGATORCONTENTUTILS_NAVIGATOR_CONTENT_UTILS_CLIENT_H_
