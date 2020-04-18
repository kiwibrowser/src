// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PUSH_MESSAGING_PUSH_CONTROLLER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PUSH_MESSAGING_PUSH_CONTROLLER_H_

#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class WebPushClient;

class PushController final : public GarbageCollected<PushController>,
                             public Supplement<LocalFrame> {
  USING_GARBAGE_COLLECTED_MIXIN(PushController);
  WTF_MAKE_NONCOPYABLE(PushController);

 public:
  static const char kSupplementName[];

  PushController(LocalFrame& frame, WebPushClient* client);

  static PushController* From(LocalFrame* frame) {
    return Supplement<LocalFrame>::From<PushController>(frame);
  }
  static WebPushClient& ClientFrom(LocalFrame*);

  void Trace(blink::Visitor* visitor) override {
    Supplement<LocalFrame>::Trace(visitor);
  }

 private:
  WebPushClient* Client() const { return client_; }

  WebPushClient* client_;
};

MODULES_EXPORT void ProvidePushControllerTo(LocalFrame& frame,
                                            WebPushClient* client);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PUSH_MESSAGING_PUSH_CONTROLLER_H_
