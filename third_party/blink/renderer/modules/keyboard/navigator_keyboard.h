// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_KEYBOARD_NAVIGATOR_KEYBOARD_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_KEYBOARD_NAVIGATOR_KEYBOARD_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class Keyboard;

// Navigator supplement which exposes keyboard related functionality.
class NavigatorKeyboard final : public GarbageCollected<NavigatorKeyboard>,
                                public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorKeyboard);
  WTF_MAKE_NONCOPYABLE(NavigatorKeyboard);

 public:
  static const char kSupplementName[];
  static Keyboard* keyboard(Navigator&);

  void Trace(blink::Visitor*) override;

 private:
  explicit NavigatorKeyboard(Navigator&);

  Member<Keyboard> keyboard_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_KEYBOARD_NAVIGATOR_KEYBOARD_H_
