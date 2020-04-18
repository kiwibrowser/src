// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cast/common/frame_id.h"

namespace media {
namespace cast {

std::ostream& operator<<(std::ostream& out, const FrameId rhs) {
  out << "F";
  if (rhs.is_null())
    return out << "<null>";
  return out << rhs.value();
}

}  // namespace cast
}  // namespace media
