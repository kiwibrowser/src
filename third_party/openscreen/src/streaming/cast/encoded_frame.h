// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STREAMING_CAST_ENCODED_FRAME_H_
#define STREAMING_CAST_ENCODED_FRAME_H_

#include <stdint.h>

#include <chrono>
#include <vector>

#include "osp_base/macros.h"
#include "platform/api/time.h"
#include "streaming/cast/frame_id.h"
#include "streaming/cast/rtp_time.h"

namespace openscreen {
namespace cast_streaming {

// A combination of metadata and data for one encoded frame.  This can contain
// audio data or video data or other.
struct EncodedFrame {
  enum Dependency : int8_t {
    // "null" value, used to indicate whether |dependency| has been set.
    UNKNOWN_DEPENDENCY,

    // Not decodable without the reference frame indicated by
    // |referenced_frame_id|.
    DEPENDENT,

    // Independently decodable.
    INDEPENDENT,

    // Independently decodable, and no future frames will depend on any frames
    // before this one.
    KEY,

    DEPENDENCY_LAST = KEY
  };

  EncodedFrame();
  ~EncodedFrame();

  EncodedFrame(EncodedFrame&&) MAYBE_NOEXCEPT;
  EncodedFrame& operator=(EncodedFrame&&) MAYBE_NOEXCEPT;

  // Copies all members except |data| to |dest|. Does not modify |dest->data|.
  void CopyMetadataTo(EncodedFrame* dest) const;

  // This frame's dependency relationship with respect to other frames.
  Dependency dependency = UNKNOWN_DEPENDENCY;

  // The label associated with this frame.  Implies an ordering relative to
  // other frames in the same stream.
  FrameId frame_id;

  // The label associated with the frame upon which this frame depends.  If
  // this frame does not require any other frame in order to become decodable
  // (e.g., key frames), |referenced_frame_id| must equal |frame_id|.
  FrameId referenced_frame_id;

  // The stream timestamp, on the timeline of the signal data.  For example, RTP
  // timestamps for audio are usually defined as the total number of audio
  // samples encoded in all prior frames.  A playback system uses this value to
  // detect gaps in the stream, and otherwise stretch the signal to match
  // playout targets.
  RtpTimeTicks rtp_timestamp;

  // The common reference clock timestamp for this frame.  This value originates
  // from a sender and is used to provide lip synchronization between streams in
  // a receiver.  Thus, in the sender context, this is set to the time at which
  // the frame was captured/recorded.  In the receiver context, this is set to
  // the target playout time.  Over a sequence of frames, this time value is
  // expected to drift with respect to the elapsed time implied by the RTP
  // timestamps; and it may not necessarily increment with precise regularity.
  platform::Clock::time_point reference_time;

  // Playout delay for this and all future frames. Used by the Adaptive
  // Playout delay extension. Non-positive values means no change.
  std::chrono::milliseconds new_playout_delay{};

  // The encoded signal data.
  std::vector<uint8_t> data;

  OSP_DISALLOW_COPY_AND_ASSIGN(EncodedFrame);
};

}  // namespace cast_streaming
}  // namespace openscreen

#endif  // STREAMING_CAST_ENCODED_FRAME_H_
