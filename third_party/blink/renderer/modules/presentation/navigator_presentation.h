// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PRESENTATION_NAVIGATOR_PRESENTATION_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PRESENTATION_NAVIGATOR_PRESENTATION_H_

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class Navigator;
class Presentation;

class NavigatorPresentation final
    : public GarbageCollected<NavigatorPresentation>,
      public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorPresentation);

 public:
  static const char kSupplementName[];

  static NavigatorPresentation& From(Navigator&);
  static Presentation* presentation(Navigator&);

  void Trace(blink::Visitor*) override;

 private:
  NavigatorPresentation();

  Presentation* presentation();

  Member<Presentation> presentation_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PRESENTATION_NAVIGATOR_PRESENTATION_H_
