// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_CLIPBOARD_NAVIGATOR_CLIPBOARD_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_CLIPBOARD_NAVIGATOR_CLIPBOARD_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class Clipboard;
class ScriptState;

class NavigatorClipboard final : public GarbageCollected<NavigatorClipboard>,
                                 public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorClipboard);
  WTF_MAKE_NONCOPYABLE(NavigatorClipboard);

 public:
  static const char kSupplementName[];
  static Clipboard* clipboard(ScriptState*, Navigator&);

  void Trace(blink::Visitor*) override;

 private:
  explicit NavigatorClipboard(Navigator&);

  Member<Clipboard> clipboard_;
};

}  // namespace blink

#endif  // NavigatorClipboard.h
