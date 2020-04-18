// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PROPERTIES_STYLE_BUILDING_UTILS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PROPERTIES_STYLE_BUILDING_UTILS_H_

#include "third_party/blink/renderer/core/style/border_image_length.h"
#include "third_party/blink/renderer/core/style/border_image_length_box.h"
#include "third_party/blink/renderer/platform/length.h"
#include "third_party/blink/renderer/platform/length_box.h"

namespace blink {
namespace StyleBuildingUtils {

inline bool borderImageLengthMatchesAllSides(
    const BorderImageLengthBox& borderImageLengthBox,
    const BorderImageLength& borderImageLength) {
  return (borderImageLengthBox.Left() == borderImageLength &&
          borderImageLengthBox.Right() == borderImageLength &&
          borderImageLengthBox.Top() == borderImageLength &&
          borderImageLengthBox.Bottom() == borderImageLength);
}
inline bool lengthMatchesAllSides(const LengthBox& lengthBox,
                                  const Length& length) {
  return (lengthBox.Left() == length && lengthBox.Right() == length &&
          lengthBox.Top() == length && lengthBox.Bottom() == length);
}

}  // namespace StyleBuildingUtils
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PROPERTIES_STYLE_BUILDING_UTILS_H_
