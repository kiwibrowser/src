// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/bitmap_glyphs_blacklist.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

#include "SkTypeface.h"

namespace blink {

namespace {

// Calibri is the only font we encountered which has embeded bitmaps and
// vector outlines for Latin glyphs. We avoid using the bitmap glyphs
// because they cause issues with uneven spacing when combined with
// subpixel positioning, see
// https://bugs.chromium.org/p/chromium/issues/detail?id=707713#c5
constexpr const char* kBitmapGlyphsBlacklist[] = {"Calibri", "Courier New"};

}  // namespace

bool BitmapGlyphsBlacklist::AvoidEmbeddedBitmapsForTypeface(
    SkTypeface* typeface) {
  CHECK(typeface);
  SkString font_family_name;
  typeface->getFamilyName(&font_family_name);
  return font_family_name.equals(kBitmapGlyphsBlacklist[0]) ||
         font_family_name.equals(kBitmapGlyphsBlacklist[1]);
}

}  // namespace blink
