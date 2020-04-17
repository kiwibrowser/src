// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/rtp_time.h"

#include <sstream>

namespace openscreen {
namespace cast_streaming {

std::ostream& operator<<(std::ostream& out, const RtpTimeDelta rhs) {
  if (rhs.value_ >= 0)
    out << "RTP+";
  else
    out << "RTP";
  return out << rhs.value_;
}

std::ostream& operator<<(std::ostream& out, const RtpTimeTicks rhs) {
  return out << "RTP@" << rhs.value_;
}

}  // namespace cast_streaming
}  // namespace openscreen
