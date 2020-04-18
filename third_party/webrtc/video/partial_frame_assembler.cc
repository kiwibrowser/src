/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "video/partial_frame_assembler.h"

#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"

namespace webrtc {

PartialFrameAssembler::PartialFrameAssembler() = default;
PartialFrameAssembler::~PartialFrameAssembler() = default;

bool PartialFrameAssembler::ApplyPartialUpdate(
    const rtc::scoped_refptr<VideoFrameBuffer>& input_buffer,
    VideoFrame* uncompressed_frame,
    const VideoFrame::PartialFrameDescription* partial_desc) {
  const int changed_rect_width = input_buffer ? input_buffer->width() : 0;
  const int changed_rect_height = input_buffer ? input_buffer->height() : 0;
  if (partial_desc == nullptr) {
    // Full update. Copy whole picture to the cached buffer. May need to
    // resize or create the cache buffer.
    if (!cached_frame_buffer_ ||
        cached_frame_buffer_->height() < input_buffer->height() ||
        cached_frame_buffer_->width() < input_buffer->width()) {
      cached_frame_buffer_ =
          I420Buffer::Create(input_buffer->width(), input_buffer->height());
    }
    cached_frame_buffer_->PasteFrom(*input_buffer->ToI420().get(), 0, 0);
  } else {
    // Have to apply partial input picture to the cached buffer.
    // Check all possible error situations.
    if (!cached_frame_buffer_) {
      RTC_LOG(LS_ERROR) << "Partial picture received but no cached full picture"
                           "present.";
      return false;
    }
    if (partial_desc->offset_x % 2 != 0 || partial_desc->offset_y % 2 != 0) {
      RTC_LOG(LS_ERROR) << "Partial picture required to be at even offset."
                           " Actual: ("
                        << partial_desc->offset_x << ", "
                        << partial_desc->offset_y << ").";
      cached_frame_buffer_ = nullptr;
      return false;
    }
    if ((changed_rect_width % 2 != 0 &&
         changed_rect_width + partial_desc->offset_x <
             cached_frame_buffer_->width()) ||
        (changed_rect_height % 2 != 0 &&
         changed_rect_height + partial_desc->offset_y <
             cached_frame_buffer_->height())) {
      RTC_LOG(LS_ERROR) << "Partial picture required to have even dimensions."
                           " Actual: "
                        << input_buffer->width() << "x"
                        << input_buffer->height() << ".";
      cached_frame_buffer_ = nullptr;
      return false;
    }
    if (partial_desc->offset_x < 0 ||
        partial_desc->offset_x + changed_rect_width >
            cached_frame_buffer_->width() ||
        partial_desc->offset_y < 0 ||
        partial_desc->offset_y + changed_rect_height >
            cached_frame_buffer_->height()) {
      RTC_LOG(LS_ERROR) << "Partial picture is outside of bounds.";
      cached_frame_buffer_ = nullptr;
      return false;
    }
    // No errors: apply new image to the cache and use the result.
    if (input_buffer) {
      cached_frame_buffer_->PasteFrom(*input_buffer->ToI420().get(),
                                      partial_desc->offset_x,
                                      partial_desc->offset_y);
    }
  }
  // Remove partial frame description, as it doesn't make sense after update
  // is applied.
  uncompressed_frame->set_partial_frame_description(absl::nullopt);
  uncompressed_frame->set_video_frame_buffer(
      I420Buffer::Copy(*cached_frame_buffer_.get()));
  return true;
}

void PartialFrameAssembler::Reset() {
  cached_frame_buffer_ = nullptr;
}

}  // namespace webrtc
