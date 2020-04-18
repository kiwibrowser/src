// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/html_rt_element.h"

#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/layout/layout_ruby_text.h"

namespace blink {

using namespace HTMLNames;

inline HTMLRTElement::HTMLRTElement(Document& document)
    : HTMLElement(rtTag, document) {}

DEFINE_NODE_FACTORY(HTMLRTElement)

LayoutObject* HTMLRTElement::CreateLayoutObject(const ComputedStyle& style) {
  if (style.Display() == EDisplay::kBlock)
    return new LayoutRubyText(this);
  return LayoutObject::CreateObject(this, style);
}

}  // namespace blink
