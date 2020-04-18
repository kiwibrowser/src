/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_STYLE_COLOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_STYLE_COLOR_H_

#include "third_party/blink/renderer/core/css_value_keywords.h"
#include "third_party/blink/renderer/platform/graphics/color.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class StyleColor {
  DISALLOW_NEW();

 public:
  StyleColor() : current_color_(true) {}
  StyleColor(Color color) : color_(color), current_color_(false) {}
  static StyleColor CurrentColor() { return StyleColor(); }

  bool IsCurrentColor() const { return current_color_; }
  Color GetColor() const {
    DCHECK(!IsCurrentColor());
    return color_;
  }

  Color Resolve(Color current_color) const {
    return current_color_ ? current_color : color_;
  }

  static Color ColorFromKeyword(CSSValueID);
  static bool IsColorKeyword(CSSValueID);
  static bool IsSystemColor(CSSValueID);

 private:
  Color color_;
  bool current_color_;
};

inline bool operator==(const StyleColor& a, const StyleColor& b) {
  if (a.IsCurrentColor() || b.IsCurrentColor())
    return a.IsCurrentColor() && b.IsCurrentColor();
  return a.GetColor() == b.GetColor();
}

inline bool operator!=(const StyleColor& a, const StyleColor& b) {
  return !(a == b);
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_STYLE_COLOR_H_
