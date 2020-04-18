// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PUSH_MESSAGING_PUSH_ERROR_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PUSH_MESSAGING_PUSH_ERROR_H_

#include "third_party/blink/public/platform/modules/push_messaging/web_push_error.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class ScriptPromiseResolver;

class PushError {
  STATIC_ONLY(PushError);

 public:
  // For CallbackPromiseAdapter.
  using WebType = const WebPushError&;
  static DOMException* Take(ScriptPromiseResolver* resolver,
                            const WebPushError& web_error);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PUSH_MESSAGING_PUSH_ERROR_H_
