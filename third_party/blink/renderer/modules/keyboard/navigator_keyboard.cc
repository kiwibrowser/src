// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/keyboard/navigator_keyboard.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/modules/keyboard/keyboard.h"

namespace blink {

// static
const char NavigatorKeyboard::kSupplementName[] = "NavigatorKeyboard";

NavigatorKeyboard::NavigatorKeyboard(Navigator& navigator)
    : Supplement<Navigator>(navigator),
      keyboard_(
          new Keyboard(GetSupplementable()->GetFrame()
                           ? GetSupplementable()->GetFrame()->GetDocument()
                           : nullptr)) {}

// static
Keyboard* NavigatorKeyboard::keyboard(Navigator& navigator) {
  NavigatorKeyboard* supplement =
      Supplement<Navigator>::From<NavigatorKeyboard>(navigator);
  if (!supplement) {
    supplement = new NavigatorKeyboard(navigator);
    ProvideTo(navigator, supplement);
  }
  return supplement->keyboard_;
}

void NavigatorKeyboard::Trace(blink::Visitor* visitor) {
  visitor->Trace(keyboard_);
  Supplement<Navigator>::Trace(visitor);
}

}  // namespace blink
