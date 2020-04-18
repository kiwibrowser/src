// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/vaapi/vp8_encoder.h"

#include "base/bits.h"

#define DVLOGF(level) DVLOG(level) << __func__ << "(): "

namespace media {

namespace {
// Keyframe period.
const size_t kKFPeriod = 3000;

// Arbitrarily chosen bitrate window size for rate control, in ms.
const int kCPBWindowSizeMs = 1500;

// Based on WebRTC's defaults.
const int kMinQP = 4;
const int kMaxQP = 112;
const int kDefaultQP = (3 * kMinQP + kMaxQP) / 4;
}  // namespace

VP8Encoder::EncodeParams::EncodeParams()
    : kf_period_frames(kKFPeriod),
      bitrate_bps(0),
      framerate(0),
      cpb_window_size_ms(kCPBWindowSizeMs),
      cpb_size_bits(0),
      initial_qp(kDefaultQP),
      min_qp(kMinQP),
      max_qp(kMaxQP),
      error_resilient_mode(false) {}

void VP8Encoder::Reset() {
  current_params_ = EncodeParams();
  reference_frames_.Clear();
  frame_num_ = 0;

  InitializeFrameHeader();
}

VP8Encoder::VP8Encoder(std::unique_ptr<Accelerator> accelerator)
    : accelerator_(std::move(accelerator)) {}

VP8Encoder::~VP8Encoder() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

bool VP8Encoder::Initialize(const gfx::Size& visible_size,
                            VideoCodecProfile profile,
                            uint32_t initial_bitrate,
                            uint32_t initial_framerate) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(profile >= VP8PROFILE_MIN && profile <= VP8PROFILE_MAX);

  DCHECK(!visible_size.IsEmpty());
  // 4:2:0 format has to be 2-aligned.
  DCHECK_EQ(visible_size.width() % 2, 0);
  DCHECK_EQ(visible_size.height() % 2, 0);

  visible_size_ = visible_size;
  coded_size_ = gfx::Size(base::bits::Align(visible_size_.width(), 16),
                          base::bits::Align(visible_size_.height(), 16));

  Reset();

  return UpdateRates(initial_bitrate, initial_framerate);
}

gfx::Size VP8Encoder::GetCodedSize() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!coded_size_.IsEmpty());

  return coded_size_;
}

size_t VP8Encoder::GetBitstreamBufferSize() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!coded_size_.IsEmpty());

  return coded_size_.GetArea();
}

size_t VP8Encoder::GetMaxNumOfRefFrames() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return kNumVp8ReferenceBuffers;
}

bool VP8Encoder::PrepareEncodeJob(EncodeJob* encode_job) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (encode_job->IsKeyframeRequested())
    frame_num_ = 0;

  if (frame_num_ == 0)
    encode_job->ProduceKeyframe();

  frame_num_++;
  frame_num_ %= current_params_.kf_period_frames;

  scoped_refptr<VP8Picture> picture = accelerator_->GetPicture(encode_job);
  DCHECK(picture);

  UpdateFrameHeader(encode_job->IsKeyframeRequested());
  *picture->frame_hdr = current_frame_hdr_;

  if (!accelerator_->SubmitFrameParameters(encode_job, current_params_, picture,
                                           reference_frames_)) {
    LOG(ERROR) << "Failed submitting frame parameters";
    return false;
  }

  UpdateReferenceFrames(picture);
  return true;
}

bool VP8Encoder::UpdateRates(uint32_t bitrate, uint32_t framerate) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (bitrate == 0 || framerate == 0)
    return false;

  if (current_params_.bitrate_bps == bitrate &&
      current_params_.framerate == framerate) {
    return true;
  }

  current_params_.bitrate_bps = bitrate;
  current_params_.framerate = framerate;

  current_params_.cpb_size_bits =
      current_params_.bitrate_bps * current_params_.cpb_window_size_ms / 1000;

  return true;
}

void VP8Encoder::InitializeFrameHeader() {
  current_frame_hdr_ = {};
  DCHECK(!visible_size_.IsEmpty());
  current_frame_hdr_.width = visible_size_.width();
  current_frame_hdr_.height = visible_size_.height();
  current_frame_hdr_.quantization_hdr.y_ac_qi = current_params_.initial_qp;
  current_frame_hdr_.show_frame = true;
  // TODO(sprang): Make this dynamic. Value based on reference implementation
  // in libyami (https://github.com/intel/libyami).
  current_frame_hdr_.loopfilter_hdr.level = 19;
}

void VP8Encoder::UpdateFrameHeader(bool keyframe) {
  current_frame_hdr_.frame_type =
      keyframe ? Vp8FrameHeader::KEYFRAME : Vp8FrameHeader::INTERFRAME;
}

void VP8Encoder::UpdateReferenceFrames(scoped_refptr<VP8Picture> picture) {
  if (current_frame_hdr_.IsKeyframe()) {
    current_frame_hdr_.refresh_last = true;
    current_frame_hdr_.refresh_golden_frame = true;
    current_frame_hdr_.refresh_alternate_frame = true;
    current_frame_hdr_.copy_buffer_to_golden =
        Vp8FrameHeader::NO_GOLDEN_REFRESH;
    current_frame_hdr_.copy_buffer_to_alternate =
        Vp8FrameHeader::NO_ALT_REFRESH;
  } else {
    // TODO(sprang): Add temporal layer support.
    current_frame_hdr_.refresh_last = true;
    current_frame_hdr_.refresh_golden_frame = false;
    current_frame_hdr_.refresh_alternate_frame = false;
    current_frame_hdr_.copy_buffer_to_golden =
        Vp8FrameHeader::COPY_LAST_TO_GOLDEN;
    current_frame_hdr_.copy_buffer_to_alternate =
        Vp8FrameHeader::COPY_GOLDEN_TO_ALT;
  }

  reference_frames_.Refresh(picture);
}

}  // namespace media
