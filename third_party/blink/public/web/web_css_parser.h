// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_CSS_PARSER_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_CSS_PARSER_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/skia/include/core/SkColor.h"

namespace blink {

class WebString;

class WebCSSParser {
 public:
  BLINK_EXPORT static bool ParseColor(SkColor*, const WebString&);
};

}  // namespace blink

#endif
