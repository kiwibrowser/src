// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_RESIZE_OBSERVER_RESIZE_OBSERVER_ENTRY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_RESIZE_OBSERVER_RESIZE_OBSERVER_ENTRY_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class Element;
class DOMRectReadOnly;
class LayoutRect;

class ResizeObserverEntry final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ResizeObserverEntry(Element* target, const LayoutRect& content_rect);

  Element* target() const { return target_; }
  DOMRectReadOnly* contentRect() const { return content_rect_; }

  void Trace(blink::Visitor*) override;

 private:
  Member<Element> target_;
  Member<DOMRectReadOnly> content_rect_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_RESIZE_OBSERVER_RESIZE_OBSERVER_ENTRY_H_
