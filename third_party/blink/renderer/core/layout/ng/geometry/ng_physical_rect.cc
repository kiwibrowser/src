// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_rect.h"

#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

NGPixelSnappedPhysicalRect NGPhysicalRect::SnapToDevicePixels() const {
  NGPixelSnappedPhysicalRect snapped_rect;
  snapped_rect.left = RoundToInt(location.left);
  snapped_rect.top = RoundToInt(location.top);
  snapped_rect.width = SnapSizeToPixel(size.width, location.left);
  snapped_rect.height = SnapSizeToPixel(size.height, location.top);

  return snapped_rect;
}

bool NGPhysicalRect::operator==(const NGPhysicalRect& other) const {
  return other.location == location && other.size == size;
}

String NGPhysicalRect::ToString() const {
  return String::Format("%s,%s %sx%s", location.left.ToString().Ascii().data(),
                        location.top.ToString().Ascii().data(),
                        size.width.ToString().Ascii().data(),
                        size.height.ToString().Ascii().data());
}

std::ostream& operator<<(std::ostream& os, const NGPhysicalRect& value) {
  return os << value.ToString();
}

}  // namespace blink
