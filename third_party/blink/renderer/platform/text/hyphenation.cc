// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/text/hyphenation.h"

#include "third_party/blink/renderer/platform/fonts/font.h"
#include "third_party/blink/renderer/platform/wtf/text/string_view.h"

namespace blink {

size_t Hyphenation::FirstHyphenLocation(const StringView& text,
                                        size_t after_index) const {
  Vector<size_t, 8> hyphen_locations = HyphenLocations(text);
  for (auto it = hyphen_locations.rbegin(); it != hyphen_locations.rend();
       ++it) {
    if (*it > after_index)
      return *it;
  }
  return 0;
}

Vector<size_t, 8> Hyphenation::HyphenLocations(const StringView& text) const {
  Vector<size_t, 8> hyphen_locations;
  size_t hyphen_location = text.length();
  if (hyphen_location <= kMinimumSuffixLength)
    return hyphen_locations;
  hyphen_location -= kMinimumSuffixLength;

  while ((hyphen_location = LastHyphenLocation(text, hyphen_location)) >=
         kMinimumPrefixLength)
    hyphen_locations.push_back(hyphen_location);

  return hyphen_locations;
}

int Hyphenation::MinimumPrefixWidth(const Font& font) {
  // If the maximum width available for the prefix before the hyphen is small,
  // then it is very unlikely that an hyphenation opportunity exists, so do not
  // bother to look for it.  These are heuristic numbers for performance added
  // in http://wkb.ug/45606
  const int kMinimumPrefixWidthNumerator = 5;
  const int kMinimumPrefixWidthDenominator = 4;
  return font.GetFontDescription().ComputedPixelSize() *
         kMinimumPrefixWidthNumerator / kMinimumPrefixWidthDenominator;
}

}  // namespace blink
