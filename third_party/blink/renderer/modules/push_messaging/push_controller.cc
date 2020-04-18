// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/push_messaging/push_controller.h"

#include "third_party/blink/public/platform/modules/push_messaging/web_push_client.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

PushController::PushController(LocalFrame& frame, WebPushClient* client)
    : Supplement<LocalFrame>(frame), client_(client) {}

WebPushClient& PushController::ClientFrom(LocalFrame* frame) {
  PushController* controller = PushController::From(frame);
  DCHECK(controller);
  WebPushClient* client = controller->Client();
  DCHECK(client);
  return *client;
}

const char PushController::kSupplementName[] = "PushController";

void ProvidePushControllerTo(LocalFrame& frame, WebPushClient* client) {
  PushController::ProvideTo(frame, new PushController(frame, client));
}

}  // namespace blink
