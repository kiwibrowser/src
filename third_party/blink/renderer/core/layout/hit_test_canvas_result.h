// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_HIT_TEST_CANVAS_RESULT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_HIT_TEST_CANVAS_RESULT_H_

#include "third_party/blink/renderer/core/dom/element.h"

namespace blink {

class CORE_EXPORT HitTestCanvasResult final
    : public GarbageCollectedFinalized<HitTestCanvasResult> {
 public:
  static HitTestCanvasResult* Create(String id, Member<Element> control) {
    return new HitTestCanvasResult(id, control);
  }

  String GetId() const;
  Element* GetControl() const;

  void Trace(blink::Visitor*);

 private:
  HitTestCanvasResult(String id, Member<Element> control);

  String id_;
  Member<Element> control_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_HIT_TEST_CANVAS_RESULT_H_
