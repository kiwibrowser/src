// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_NFC_NAVIGATOR_NFC_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_NFC_NAVIGATOR_NFC_H_

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class NFC;
class Navigator;

class NavigatorNFC final : public GarbageCollected<NavigatorNFC>,
                           public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorNFC);

 public:
  static const char kSupplementName[];

  // Gets, or creates, NavigatorNFC supplement on Navigator.
  static NavigatorNFC& From(Navigator&);

  static NFC* nfc(Navigator&);

  void Trace(blink::Visitor*) override;
  void TraceWrappers(ScriptWrappableVisitor*) const override;

 private:
  explicit NavigatorNFC(Navigator&);

  TraceWrapperMember<NFC> nfc_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_NFC_NAVIGATOR_NFC_H_
