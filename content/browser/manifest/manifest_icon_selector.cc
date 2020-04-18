// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/manifest_icon_selector.h"

#include <limits>

#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/blink/public/common/mime_util/mime_util.h"

namespace content {

// static
GURL ManifestIconSelector::FindBestMatchingIcon(
    const std::vector<blink::Manifest::Icon>& icons,
    int ideal_icon_size_in_px,
    int minimum_icon_size_in_px,
    blink::Manifest::Icon::IconPurpose purpose) {
  DCHECK(minimum_icon_size_in_px <= ideal_icon_size_in_px);

  // Icon with exact matching size has priority over icon with size "any", which
  // has priority over icon with closest matching size.
  int latest_size_any_index = -1;
  int closest_size_match_index = -1;
  int best_delta_in_size = std::numeric_limits<int>::min();

  for (size_t i = 0; i < icons.size(); ++i) {
    const auto& icon = icons[i];

    // Check for supported image MIME types.
    if (!icon.type.empty() &&
        !blink::IsSupportedImageMimeType(base::UTF16ToUTF8(icon.type))) {
      continue;
    }

    // Check for icon purpose.
    if (!base::ContainsValue(icon.purpose, purpose))
      continue;

    // Check for size constraints.
    for (const gfx::Size& size : icon.sizes) {
      // Check for size "any". Return this icon if no better one is found.
      if (size.IsEmpty()) {
        latest_size_any_index = i;
        continue;
      }

      // Check for squareness.
      if (size.width() != size.height())
        continue;

      // Check for minimum size.
      if (size.width() < minimum_icon_size_in_px)
        continue;

      // Check for ideal size. Return this icon immediately.
      if (size.width() == ideal_icon_size_in_px)
        return icon.src;

      // Check for closest match.
      int delta = size.width() - ideal_icon_size_in_px;

      // Smallest icon larger than ideal size has priority over largest icon
      // smaller than ideal size.
      if (best_delta_in_size > 0 && delta < 0)
        continue;

      if ((best_delta_in_size > 0 && delta < best_delta_in_size) ||
          (best_delta_in_size < 0 && delta > best_delta_in_size)) {
        closest_size_match_index = i;
        best_delta_in_size = delta;
      }
    }
  }

  if (latest_size_any_index != -1)
    return icons[latest_size_any_index].src;
  else if (closest_size_match_index != -1)
    return icons[closest_size_match_index].src;
  else
    return GURL();
}

}  // namespace content
