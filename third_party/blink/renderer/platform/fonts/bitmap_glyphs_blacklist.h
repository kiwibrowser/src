// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_BITMAP_GLYPHS_BLACKLIST_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_BITMAP_GLYPHS_BLACKLIST_H_

#include "third_party/blink/renderer/platform/platform_export.h"

class SkTypeface;

namespace blink {

class PLATFORM_EXPORT BitmapGlyphsBlacklist {
 public:
  static bool AvoidEmbeddedBitmapsForTypeface(SkTypeface*);
};

}  // namespace blink

#endif
