// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/css_color_value.h"

#include "third_party/blink/renderer/core/css/css_value_pool.h"

namespace blink {
namespace cssvalue {

CSSColorValue* CSSColorValue::Create(RGBA32 color) {
  // These are the empty and deleted values of the hash table.
  if (color == Color::kTransparent)
    return CssValuePool().TransparentColor();
  if (color == Color::kWhite)
    return CssValuePool().WhiteColor();
  // Just because it is common.
  if (color == Color::kBlack)
    return CssValuePool().BlackColor();

  CSSValuePool::ColorValueCache::AddResult entry =
      CssValuePool().GetColorCacheEntry(color);
  if (entry.is_new_entry)
    entry.stored_value->value = new CSSColorValue(color);
  return entry.stored_value->value;
}

}  // namespace cssvalue
}  // namespace blink
