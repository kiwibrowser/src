// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_bfc_rect.h"

namespace blink {

bool NGBfcRect::IsEmpty() const {
  return start_offset == end_offset;
}

bool NGBfcRect::operator==(const NGBfcRect& other) const {
  return start_offset == other.start_offset && end_offset == other.end_offset;
}

}  // namespace blink
