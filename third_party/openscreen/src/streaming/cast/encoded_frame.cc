// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/encoded_frame.h"

namespace openscreen {
namespace cast_streaming {

EncodedFrame::EncodedFrame() = default;
EncodedFrame::~EncodedFrame() = default;

EncodedFrame::EncodedFrame(EncodedFrame&&) MAYBE_NOEXCEPT = default;
EncodedFrame& EncodedFrame::operator=(EncodedFrame&&) MAYBE_NOEXCEPT = default;

void EncodedFrame::CopyMetadataTo(EncodedFrame* dest) const {
  dest->dependency = this->dependency;
  dest->frame_id = this->frame_id;
  dest->referenced_frame_id = this->referenced_frame_id;
  dest->rtp_timestamp = this->rtp_timestamp;
  dest->reference_time = this->reference_time;
  dest->new_playout_delay = this->new_playout_delay;
}

}  // namespace cast_streaming
}  // namespace openscreen
