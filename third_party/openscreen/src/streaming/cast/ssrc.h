// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STREAMING_CAST_SSRC_H_
#define STREAMING_CAST_SSRC_H_

#include <stdint.h>

namespace openscreen {
namespace cast_streaming {

// A Synchronization Source is a 32-bit opaque identifier used in RTP packets
// for identifying the source (or recipient) of a logical sequence of encoded
// audio/video frames. In other words, an audio stream will have one sender SSRC
// and a video stream will have a different sender SSRC.
using Ssrc = uint32_t;

// Computes a new SSRC that will be used to uniquely identify an RTP stream. The
// |higher_priority| argument, if true, will generate an SSRC that causes the
// system to use a higher priority when scheduling data transmission. Generally,
// this is set to true for audio streams and false for video streams.
Ssrc GenerateSsrc(bool higher_priority);

// Returns a value indicating how to prioritize data transmission for a stream
// with |ssrc_a| versus a stream with |ssrc_b|:
//
//   ret < 0: Stream |ssrc_a| has higher priority.
//   ret == 0: Equal priority.
//   ret > 0: Stream |ssrc_b| has higher priority.
int ComparePriority(Ssrc ssrc_a, Ssrc ssrc_b);

}  // namespace cast_streaming
}  // namespace openscreen

#endif  // STREAMING_CAST_SSRC_H_
