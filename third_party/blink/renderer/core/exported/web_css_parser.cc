// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/web/web_css_parser.h"

#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/core/css/parser/css_parser.h"
#include "third_party/blink/renderer/platform/graphics/color.h"

namespace blink {

bool WebCSSParser::ParseColor(SkColor* web_color,
                              const WebString& color_string) {
  Color color = Color(*web_color);
  bool success = CSSParser::ParseColor(color, color_string, true);
  *web_color = color.Rgb();
  return success;
}

}  // namespace blink
