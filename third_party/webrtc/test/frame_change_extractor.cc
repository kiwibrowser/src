/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "test/frame_change_extractor.h"

#include <algorithm>

#include "api/video/i420_buffer.h"
#include "rtc_base/checks.h"

namespace webrtc {

FrameChangeExtractor::FrameChangeExtractor() = default;
FrameChangeExtractor::~FrameChangeExtractor() = default;

void FrameChangeExtractor::SetSource(
    rtc::VideoSourceInterface<VideoFrame>* video_source) {
  rtc::CritScope lock(&source_crit_);

  rtc::VideoSourceInterface<VideoFrame>* old_source = nullptr;
  old_source = source_;
  source_ = video_source;
  if (old_source != video_source && old_source != nullptr) {
    old_source->RemoveSink(this);
  }
  if (video_source) {
    source_->AddOrUpdateSink(this, rtc::VideoSinkWants());
  }
}

void FrameChangeExtractor::OnFrame(const VideoFrame& frame) {
  // Currently only supports I420 buffers.
  RTC_CHECK(frame.video_frame_buffer()->type() ==
            VideoFrameBuffer::Type::kI420);

  rtc::CritScope lock(&sink_crit_);

  if (!sink_)
    return;

  webrtc::I420BufferInterface* current_buffer =
      frame.video_frame_buffer()->ToI420();
  int min_row, min_col, max_row, max_col;
  if (!last_frame_buffer_ ||
      current_buffer->width() != last_frame_buffer_->width() ||
      current_buffer->height() != last_frame_buffer_->height()) {
    // New resolution - fully new picture.
    min_row = min_col = 0;
    max_row = current_buffer->height() - 1;
    max_col = current_buffer->width() - 1;
  } else {
    min_row = frame.width();
    min_col = frame.height();
    max_row = 0;
    max_col = 0;
    // Detect changed rect.
    for (int row = 0; row < frame.height(); ++row) {
      for (int col = 0; col < frame.width(); ++col) {
        int uv_row = row / 2;
        int uv_col = col / 2;
        if (current_buffer
                    ->DataU()[uv_row * current_buffer->StrideU() + uv_col] !=
                last_frame_buffer_
                    ->DataU()[uv_row * last_frame_buffer_->StrideU() +
                              uv_col] ||
            current_buffer
                    ->DataV()[uv_row * current_buffer->StrideV() + uv_col] !=
                last_frame_buffer_
                    ->DataV()[uv_row * last_frame_buffer_->StrideV() +
                              uv_col] ||
            current_buffer->DataY()[row * current_buffer->StrideY() + col] !=
                last_frame_buffer_
                    ->DataY()[row * last_frame_buffer_->StrideY() + col]) {
          min_row = std::min(min_row, row);
          max_row = std::max(max_row, row);
          min_col = std::min(min_col, col);
          max_col = std::max(max_col, col);
        }
      }
    }
  }
  if (max_row < min_row || max_col < min_col) {
    min_col = min_row = 0;
    max_col = max_row = -1;
  }
  // Expand changed rect to accommodate subsampled UV plane.
  if (min_row % 2)
    --min_row;
  if (min_col % 2)
    --min_row;
  if (max_row < frame.width() && max_row % 2 == 0)
    ++max_row;
  if (max_col < frame.height() && max_col % 2 == 0)
    ++max_col;
  VideoFrame::PartialFrameDescription part_desc;
  part_desc.offset_x = min_col;
  part_desc.offset_y = min_row;
  int changed_width = max_col - min_col + 1;
  int changed_height = max_row - min_row + 1;
  last_frame_buffer_ = current_buffer;
  VideoFrame copy = frame;
  if (changed_width > 0 && changed_height > 0) {
    rtc::scoped_refptr<I420Buffer> changed_buffer =
        I420Buffer::Create(changed_width, changed_height);
    changed_buffer->CropAndScaleFrom(*current_buffer, part_desc.offset_x,
                                     part_desc.offset_y, changed_width,
                                     changed_height);
    copy.set_video_frame_buffer(changed_buffer);
  } else {
    copy.set_video_frame_buffer(nullptr);
  }
  if (changed_width < frame.width() || changed_height < frame.height()) {
    copy.set_partial_frame_description(part_desc);
  }
  copy.set_cache_buffer_for_partial_updates(true);
  sink_->OnFrame(copy);
}

void FrameChangeExtractor::AddOrUpdateSink(
    rtc::VideoSinkInterface<VideoFrame>* sink,
    const rtc::VideoSinkWants& wants) {
  RTC_CHECK(wants.partial_frames);
  {
    rtc::CritScope lock(&source_crit_);
    if (source_) {
      source_->AddOrUpdateSink(this, wants);
    }
  }
  {
    rtc::CritScope lock(&sink_crit_);
    // Several sinks are unsupported.
    RTC_CHECK(sink_ == nullptr || sink_ == sink);
    sink_ = sink;
  }
}

void FrameChangeExtractor::RemoveSink(
    rtc::VideoSinkInterface<VideoFrame>* sink) {
  {
    rtc::CritScope lock(&source_crit_);
    if (source_) {
      source_->RemoveSink(this);
    }
  }
  {
    rtc::CritScope lock(&sink_crit_);
    sink_ = nullptr;
  }
}

}  // namespace webrtc
