// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/util/display_util.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/macros.h"

namespace display {

namespace {

// A list of bogus sizes in mm that should be ignored.
// See crbug.com/136533. The first element maintains the minimum
// size required to be valid size.
const int kInvalidDisplaySizeList[][2] = {
  {40, 30},
  {50, 40},
  {160, 90},
  {160, 100},
};

}  // namespace

bool IsDisplaySizeBlackListed(const gfx::Size& physical_size) {
  // Ignore if the reported display is smaller than minimum size.
  if (physical_size.width() <= kInvalidDisplaySizeList[0][0] ||
      physical_size.height() <= kInvalidDisplaySizeList[0][1]) {
    VLOG(1) << "Smaller than minimum display size";
    return true;
  }
  for (size_t i = 1; i < arraysize(kInvalidDisplaySizeList); ++i) {
    const gfx::Size size(kInvalidDisplaySizeList[i][0],
                         kInvalidDisplaySizeList[i][1]);
    if (physical_size == size) {
      VLOG(1) << "Black listed display size detected:" << size.ToString();
      return true;
    }
  }
  return false;
}

int64_t GenerateDisplayID(uint16_t manufacturer_id,
                          uint32_t product_code_hash,
                          uint8_t output_index) {
  return ((static_cast<int64_t>(manufacturer_id) << 40) |
          (static_cast<int64_t>(product_code_hash) << 8) | output_index);
}

}  // namespace display
