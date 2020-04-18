// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/snooper_node.h"

#include <algorithm>
#include <cmath>

#include "base/bind.h"
#include "base/numerics/checked_math.h"
#include "base/trace_event/trace_event.h"
#include "media/base/audio_bus.h"
#include "media/base/audio_timestamp_helper.h"

using Helper = media::AudioTimestampHelper;

namespace audio {

namespace {

// The delay buffer size is chosen to be a very conservative maximum, just to
// make sure there is an upper bound in-place so that the buffer won't grow
// indefinitely. In most normal cases, reads will cause the delay buffer to
// automatically prune its recording down to well under this maximum (e.g.,
// around 100 milliseconds of audio).
constexpr base::TimeDelta kDelayBufferSize =
    base::TimeDelta::FromMilliseconds(1000);

// A frequency at which people cannot discern tones that differ by 1 Hz. This is
// based on research that shows people can discern tones only when they are more
// than 1 Hz away from 500 Hz. Thus, assume people definitely can't discern
// tones 1 Hz away from 1000 Hz.
constexpr int kStepBasisHz = 1000;

// The number of frames the resampler should request at a time. Three kernel's
// worth is an arbitrary choice, but performs well since the lock guarding
// access to the delay buffer is only held a reasonably short time during the
// data extraction.
constexpr int kResamplerRequestSize = 3 * media::SincResampler::kKernelSize;

}  // namespace

// static
constexpr SnooperNode::FrameTicks SnooperNode::kNullPosition;

// static
constexpr SnooperNode::FrameTicks SnooperNode::kWriteStartPosition;

SnooperNode::SnooperNode(const media::AudioParameters& input_params,
                         const media::AudioParameters& output_params)
    : input_params_(input_params),
      output_params_(output_params),
      input_bus_duration_(
          Helper::FramesToTime(input_params_.frames_per_buffer(),
                               input_params_.sample_rate())),
      output_bus_duration_(
          Helper::FramesToTime(output_params_.frames_per_buffer(),
                               output_params_.sample_rate())),
      perfect_io_ratio_(static_cast<double>(input_params_.sample_rate()) /
                        output_params_.sample_rate()),
      buffer_(
          Helper::TimeToFrames(kDelayBufferSize, input_params_.sample_rate())),
      write_position_(kNullPosition),
      read_position_(kNullPosition),
      correction_fps_(0),
      resampler_(
          // For efficiency, a |channel_mix_strategy_| is chosen so that the
          // resampler is always processing the fewest number of channels.
          std::min(input_params_.channels(), output_params_.channels()),
          perfect_io_ratio_,
          kResamplerRequestSize,
          base::BindRepeating(&SnooperNode::ReadFromDelayBuffer,
                              base::Unretained(this))),
      channel_mix_strategy_(
          (input_params_.channel_layout() == output_params_.channel_layout())
              ? kNone
              : ((output_params_.channels() < input_params_.channels())
                     ? kBefore
                     : kAfter)),
      channel_mixer_(input_params_.channel_layout(),
                     output_params_.channel_layout()) {
  TRACE_EVENT2("audio", "SnooperNode::SnooperNode", "input_params",
               input_params.AsHumanReadableString(), "output_params",
               output_params.AsHumanReadableString());

  // Prime the resampler with silence to keep the calculations in Render()
  // simple.
  resampler_.PrimeWithSilence();

  // If channel mixing is to be performed after resampling, allocate a buffer to
  // hold the resampler's output.
  if (channel_mix_strategy_ == kAfter) {
    mix_bus_ = media::AudioBus::Create(input_params_.channels(),
                                       output_params_.frames_per_buffer());
  }
}

SnooperNode::~SnooperNode() = default;

void SnooperNode::OnData(const media::AudioBus& input_bus,
                         base::TimeTicks reference_time,
                         double volume) {
  DCHECK_EQ(input_bus.channels(), input_params_.channels());
  DCHECK_EQ(input_bus.frames(), input_params_.frames_per_buffer());

  TRACE_EVENT_WITH_FLOW2("audio", "SnooperNode::OnData", this,
                         TRACE_EVENT_FLAG_FLOW_IN | TRACE_EVENT_FLAG_FLOW_OUT,
                         "reference_time (bogo-μs)",
                         reference_time.since_origin().InMicroseconds(),
                         "write_position", write_position_);

  base::AutoLock scoped_lock(lock_);

  // If this is the first OnData() call, just set the starting read position.
  // Otherwise, check whether a gap (i.e., missing piece) in the recording flow
  // has occurred, and skip the write position forward if necessary.
  if (write_position_ == kNullPosition) {
    write_position_ = kWriteStartPosition;
  } else {
    const base::TimeDelta delta = reference_time - write_reference_time_;
    if (delta >= input_bus_duration_) {
      TRACE_EVENT_INSTANT1("audio", "SnooperNode Input Gap",
                           TRACE_EVENT_SCOPE_THREAD, "gap (μs)",
                           delta.InMicroseconds());
      write_position_ +=
          Helper::TimeToFrames(delta, input_params_.sample_rate());
    }
  }

  buffer_.Write(write_position_, input_bus, volume);

  write_position_ += input_bus.frames();
  write_reference_time_ = reference_time + input_bus_duration_;
}

void SnooperNode::Render(base::TimeTicks reference_time,
                         media::AudioBus* output_bus) {
  DCHECK_EQ(output_bus->channels(), output_params_.channels());
  DCHECK_EQ(output_bus->frames(), output_params_.frames_per_buffer());

  TRACE_EVENT_WITH_FLOW1("audio", "SnooperNode::Render", this,
                         TRACE_EVENT_FLAG_FLOW_IN | TRACE_EVENT_FLAG_FLOW_OUT,
                         "reference_time (bogo-μs)",
                         reference_time.since_origin().InMicroseconds());

  // Use the difference in reference times between OnData() and Render() to
  // estimate the position of the audio about to come out of the resampler.
  lock_.Acquire();
  const FrameTicks estimated_output_position =
      (write_position_ == kNullPosition)
          ? kNullPosition
          : (write_position_ +
             Helper::TimeToFrames(reference_time - write_reference_time_,
                                  input_params_.sample_rate()));
  lock_.Release();

  // If recording has not started, just output silence.
  if (estimated_output_position == kNullPosition) {
    output_bus->Zero();
    return;
  }

  // If this is the first Render() call after recording started, just initialize
  // the starting read position. For all successive calls, adjust the resampler
  // to account for drift, and also handle any significant time gaps between
  // Render() calls.
  if (read_position_ == kNullPosition) {
    // Walk backwards from the estimated output position to initialize the read
    // position.
    read_position_ =
        estimated_output_position + std::lround(resampler_.BufferedFrames());
    DCHECK_EQ(correction_fps_, 0);
  } else {
    const base::TimeDelta delta = reference_time - render_reference_time_;
    if (delta < output_bus_duration_) {  // Normal case: No gap.
      // Compute the drift, which is the number of frames the resampler is
      // behind in reading from the delay buffer. This calculation also accounts
      // for the frames buffered within the resampler.
      const int64_t actual_output_position =
          read_position_ - std::lround(resampler_.BufferedFrames());
      const int drift = base::checked_cast<int>(estimated_output_position -
                                                actual_output_position);
      TRACE_COUNTER_ID1("audio", "SnooperNode Drift", this, drift);

      // The goal is to have zero drift, and the target is to achieve that goal
      // in approximately one second.
      const int target_correction_fps = drift;  // Drift divided by 1 second.

      // The minimum amount to step-up or step-down the correction rate. Using
      // this prevents excessive "churn" within the resampler, where otherwise
      // it would be recomputing its convolution kernel too often.
      const int fps_step = input_params_.sample_rate() / kStepBasisHz;

      // Adjust the correction rate (and resampling ratio) based on how
      // different the target correction FPS is from the current correction
      // FPS. If more than two steps away, make an aggressive adjustment. If
      // only more than one step away, nudge the current rate by just one
      // step. Otherwise, leave the current rate unchanged.
      const int diff = target_correction_fps - correction_fps_;
      if (std::abs(diff) > 2 * fps_step) {
        UpdateCorrectionRate(target_correction_fps);
      } else if (diff > fps_step) {
        UpdateCorrectionRate(correction_fps_ + fps_step);
      } else if (diff < -fps_step) {
        UpdateCorrectionRate(correction_fps_ - fps_step);
      } else {
        // No correction necessary.
      }
    } else /* if (delta >= threshold) */ {  // Gap detected.
      TRACE_EVENT_INSTANT1("audio", "SnooperNode Render Gap",
                           TRACE_EVENT_SCOPE_THREAD, "gap (μs)",
                           delta.InMicroseconds());

      // Rather than flush and re-prime the resampler, just skip-ahead its next
      // read-from position.
      read_position_ +=
          Helper::TimeToFrames(delta, input_params_.sample_rate());

      // This special event casts doubt on the validity of the current
      // correction rates. The system is likely to behave differently going
      // forward. Thus, set a zero correction rate.
      UpdateCorrectionRate(0);
    }

    TRACE_COUNTER_ID1("audio", "SnooperNode Correction FPS", this,
                      correction_fps_);
  }

  // Perform resampling and also channel mixing, if required. The resampler will
  // call ReadFromDelayBuffer(), as needed, to supply itself with more input
  // data; and this will move the |read_position_| forward.
  if (channel_mix_strategy_ == kAfter) {
    resampler_.Resample(mix_bus_->frames(), mix_bus_.get());
    channel_mixer_.Transform(mix_bus_.get(), output_bus);
  } else {
    resampler_.Resample(output_bus->frames(), output_bus);
  }

  render_reference_time_ = reference_time + output_bus_duration_;
}

void SnooperNode::UpdateCorrectionRate(int correction_fps) {
  correction_fps_ = correction_fps;
  const double ratio_adjustment =
      static_cast<double>(correction_fps_) / input_params_.sample_rate();
  DCHECK_GT(ratio_adjustment, -perfect_io_ratio_);
  resampler_.SetRatio(perfect_io_ratio_ + ratio_adjustment);
}

void SnooperNode::ReadFromDelayBuffer(int ignored,
                                      media::AudioBus* resampler_bus) {
  DCHECK_NE(read_position_, kNullPosition);
  const int frames_to_read = resampler_bus->frames();
  TRACE_EVENT2("audio", "SnooperNode::ReadFromDelayBuffer", "read_position",
               read_position_, "frames", frames_to_read);

  if (channel_mix_strategy_ == kBefore) {
    DCHECK_EQ(resampler_bus->channels(), output_params_.channels());

    // Reallocate the |mix_bus_| if needed.
    if (!mix_bus_ || mix_bus_->frames() < frames_to_read) {
      mix_bus_ = nullptr;  // Free memory before allocating more.
      mix_bus_ =
          media::AudioBus::Create(input_params_.channels(), frames_to_read);
    }

    // Do the read and also channel remix before resampling.
    lock_.Acquire();
    buffer_.Read(read_position_, frames_to_read, mix_bus_.get());
    lock_.Release();
    channel_mixer_.TransformPartial(mix_bus_.get(), frames_to_read,
                                    resampler_bus);
  } else {
    DCHECK_EQ(resampler_bus->channels(), input_params_.channels());
    lock_.Acquire();
    buffer_.Read(read_position_, frames_to_read, resampler_bus);
    lock_.Release();
  }

  read_position_ += frames_to_read;
}

}  // namespace audio
