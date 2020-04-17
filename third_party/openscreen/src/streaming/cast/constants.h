// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STREAMING_CAST_CONSTANTS_H_
#define STREAMING_CAST_CONSTANTS_H_

////////////////////////////////////////////////////////////////////////////////
// NOTE: This file should only contain constants that are reasonably globally
// used (i.e., by many modules, and in all or nearly all subdirs).  Do NOT add
// non-POD constants, functions, interfaces, or any logic to this module.
////////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <ratio>

namespace openscreen {
namespace cast_streaming {

// Target number of milliseconds between the sending of RTCP reports.  Both
// senders and receivers regularly send RTCP reports to their peer.
constexpr std::chrono::milliseconds kRtcpReportInterval(500);

// This is an important system-wide constant.  This limits how much history
// the implementation must retain in order to process the acknowledgements of
// past frames.
//
// This value is carefully choosen such that it fits in the 8-bits range for
// frame IDs. It is also less than half of the full 8-bits range such that
// logic can handle wrap around and compare two frame IDs meaningfully.
constexpr int kMaxUnackedFrames = 120;

// The spec declares RTP timestamps must always have a timebase of 90000 ticks
// per second for video.
using kVideoTimebase = std::ratio<1, 90000>;

}  // namespace cast_streaming
}  // namespace openscreen

#endif  // STREAMING_CAST_CONSTANTS_H_
