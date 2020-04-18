// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/audio_fader.h"

#include <string.h>

#include "base/logging.h"
#include "media/base/audio_bus.h"

namespace chromecast {
namespace media {

AudioFader::AudioFader(Source* source, int num_channels, int fade_frames)
    : source_(source),
      fade_frames_(fade_frames),
      state_(State::kSilent),
      fade_buffer_(::media::AudioBus::Create(num_channels, fade_frames)),
      buffered_frames_(0),
      fade_frames_remaining_(0) {
  DCHECK(source_);
  DCHECK_GE(fade_frames_, 0);
}

AudioFader::~AudioFader() = default;

int AudioFader::FramesNeededFromSource(int num_fill_frames) const {
  DCHECK_GE(num_fill_frames, 0);
  DCHECK_GE(fade_frames_, buffered_frames_);
  return num_fill_frames + fade_frames_ - buffered_frames_;
}

int AudioFader::FillFrames(int num_frames, ::media::AudioBus* buffer) {
  DCHECK(buffer);
  DCHECK_EQ(buffer->channels(), fade_buffer_->channels());

  // First, copy data from buffered_frames_.
  int filled_frames = std::min(buffered_frames_, num_frames);
  fade_buffer_->CopyPartialFramesTo(0, filled_frames, 0, buffer);
  buffered_frames_ -= filled_frames;
  num_frames -= filled_frames;

  // Move data in fade_buffer_ to start.
  for (int c = 0; c < fade_buffer_->channels(); ++c) {
    float* channel_data = fade_buffer_->channel(c);
    memmove(channel_data, channel_data + filled_frames,
            buffered_frames_ * sizeof(float));
  }

  if (num_frames > 0) {
    // Still need more frames; ask source to fill.
    int extra_fill =
        source_->FillFaderFrames(buffer, filled_frames, num_frames);
    filled_frames += extra_fill;
    num_frames -= extra_fill;
  }
  // Refill fade_buffer_ from source.
  buffered_frames_ += source_->FillFaderFrames(
      fade_buffer_.get(), buffered_frames_, fade_frames_ - buffered_frames_);

  const bool complete = (num_frames == 0 && buffered_frames_ == fade_frames_);
  if (complete) {
    CompleteFill(buffer, filled_frames);
  } else {
    IncompleteFill(buffer, filled_frames);
  }
  return filled_frames;
}

void AudioFader::CompleteFill(::media::AudioBus* buffer, int filled_frames) {
  switch (state_) {
    case State::kSilent:
      // Fade in.
      state_ = State::kFadingIn;
      fade_frames_remaining_ = fade_frames_;
      break;
    case State::kFadingIn:
      // Continue fading in.
      break;
    case State::kPlaying:
      // Nothing to do in this case.
      return;
    case State::kFadingOut:
      // Fade back in.
      state_ = State::kFadingIn;
      fade_frames_remaining_ =
          std::max(0, fade_frames_ - fade_frames_remaining_ - 1);
      break;
  }
  FadeIn(buffer, filled_frames);
}

void AudioFader::IncompleteFill(::media::AudioBus* buffer, int filled_frames) {
  switch (state_) {
    case State::kSilent:
      // Remain silent.
      buffered_frames_ = 0;
      buffer->ZeroFramesPartial(0, filled_frames);
      return;
    case State::kFadingIn:
      // Fade back out.
      state_ = State::kFadingOut;
      fade_frames_remaining_ =
          std::max(0, fade_frames_ - fade_frames_remaining_ - 1);
      break;
    case State::kPlaying:
      // Fade out.
      state_ = State::kFadingOut;
      fade_frames_remaining_ = fade_frames_;
      break;
    case State::kFadingOut:
      // Continue fading out.
      break;
  }
  FadeOut(buffer, filled_frames);
}

void AudioFader::FadeIn(::media::AudioBus* buffer, int filled_frames) {
  DCHECK(state_ == State::kFadingIn);

  for (int c = 0; c < buffer->channels(); ++c) {
    float* channel_data = buffer->channel(c);
    for (int f = 0; f < filled_frames && f <= fade_frames_remaining_; ++f) {
      float fade_multiplier =
          1.0 - (fade_frames_remaining_ - f) / static_cast<float>(fade_frames_);

      channel_data[f] *= fade_multiplier;
    }
  }
  fade_frames_remaining_ = std::max(0, fade_frames_remaining_ - filled_frames);

  if (fade_frames_remaining_ == 0) {
    state_ = State::kPlaying;
  }
}

void AudioFader::FadeOut(::media::AudioBus* buffer, int filled_frames) {
  DCHECK(state_ == State::kFadingOut);

  for (int c = 0; c < buffer->channels(); ++c) {
    float* channel_data = buffer->channel(c);
    for (int f = 0; f < filled_frames && f <= fade_frames_remaining_; ++f) {
      float fade_multiplier =
          (fade_frames_remaining_ - f) / static_cast<float>(fade_frames_);

      channel_data[f] *= fade_multiplier;
    }
  }
  if (filled_frames > fade_frames_remaining_) {
    buffer->ZeroFramesPartial(fade_frames_remaining_,
                              filled_frames - fade_frames_remaining_);
  }

  fade_frames_remaining_ = std::max(0, fade_frames_remaining_ - filled_frames);

  if (fade_frames_remaining_ == 0) {
    state_ = State::kSilent;
    buffered_frames_ = 0;
  }
}

}  // namespace media
}  // namespace chromecast
