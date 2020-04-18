// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/audio_decoder_for_mixer.h"

#include <algorithm>
#include <limits>

#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "chromecast/base/task_runner_impl.h"
#include "chromecast/media/cma/backend/media_pipeline_backend_for_mixer.h"
#include "chromecast/media/cma/base/decoder_buffer_adapter.h"
#include "chromecast/media/cma/base/decoder_buffer_base.h"
#include "chromecast/public/media/cast_decoder_buffer.h"
#include "media/base/audio_bus.h"
#include "media/base/channel_layout.h"
#include "media/base/decoder_buffer.h"
#include "media/base/sample_format.h"
#include "media/filters/audio_renderer_algorithm.h"

#define TRACE_FUNCTION_ENTRY0() TRACE_EVENT0("cma", __FUNCTION__)

#define TRACE_FUNCTION_ENTRY1(arg1) \
  TRACE_EVENT1("cma", __FUNCTION__, #arg1, arg1)

#define TRACE_FUNCTION_ENTRY2(arg1, arg2) \
  TRACE_EVENT2("cma", __FUNCTION__, #arg1, arg1, #arg2, arg2)

namespace chromecast {
namespace media {

namespace {

const int kNumChannels = 2;
const int kDefaultFramesPerBuffer = 1024;
const int kSilenceBufferFrames = 2048;
const int kMaxOutputMs = 20;
const int kMillisecondsPerSecond = 1000;

const double kPlaybackRateEpsilon = 0.001;

const CastAudioDecoder::OutputFormat kDecoderSampleFormat =
    CastAudioDecoder::kOutputPlanarFloat;

const int64_t kMicrosecondsPerSecond = 1000 * 1000;
const int64_t kInvalidTimestamp = std::numeric_limits<int64_t>::min();

const int64_t kNoPendingOutput = -1;

}  // namespace

// static
bool MediaPipelineBackend::AudioDecoder::RequiresDecryption() {
  return true;
}

AudioDecoderForMixer::RateShifterInfo::RateShifterInfo(float playback_rate)
    : rate(playback_rate), input_frames(0), output_frames(0) {}

AudioDecoderForMixer::AudioDecoderForMixer(
    MediaPipelineBackendForMixer* backend)
    : backend_(backend),
      task_runner_(backend->GetTaskRunner()),
      delegate_(nullptr),
      pending_buffer_complete_(false),
      got_eos_(false),
      pushed_eos_(false),
      mixer_error_(false),
      rate_shifter_output_(
          ::media::AudioBus::Create(kNumChannels, kDefaultFramesPerBuffer)),
      first_push_pts_(kInvalidTimestamp),
      last_push_pts_(kInvalidTimestamp),
      last_push_timestamp_(kInvalidTimestamp),
      last_push_pts_length_(0),
      paused_pts_(kInvalidTimestamp),
      pending_output_frames_(kNoPendingOutput),
      volume_multiplier_(1.0f),
      pool_(new ::media::AudioBufferMemoryPool()),
      weak_factory_(this) {
  TRACE_FUNCTION_ENTRY0();
  DCHECK(backend_);
  DCHECK(task_runner_.get());
  DCHECK(task_runner_->BelongsToCurrentThread());
}

AudioDecoderForMixer::~AudioDecoderForMixer() {
  TRACE_FUNCTION_ENTRY0();
  DCHECK(task_runner_->BelongsToCurrentThread());
}

void AudioDecoderForMixer::SetDelegate(
    MediaPipelineBackend::Decoder::Delegate* delegate) {
  DCHECK(delegate);
  delegate_ = delegate;
}

void AudioDecoderForMixer::Initialize() {
  TRACE_FUNCTION_ENTRY0();
  DCHECK(delegate_);
  stats_ = Statistics();
  pending_buffer_complete_ = false;
  got_eos_ = false;
  pushed_eos_ = false;
  first_push_pts_ = kInvalidTimestamp;
  last_push_pts_ = kInvalidTimestamp;
  last_push_timestamp_ = kInvalidTimestamp;
  last_push_pts_length_ = 0;
  paused_pts_ = kInvalidTimestamp;
  pending_output_frames_ = kNoPendingOutput;

  last_mixer_delay_.timestamp_microseconds = kInvalidTimestamp;
  last_mixer_delay_.delay_microseconds = 0;
}

bool AudioDecoderForMixer::Start(int64_t playback_start_timestamp) {
  TRACE_FUNCTION_ENTRY0();
  DCHECK(IsValidConfig(config_));
  mixer_input_.reset(new BufferingMixerSource(
      this, config_.samples_per_second, backend_->Primary(),
      backend_->DeviceId(), backend_->ContentType(), config_.playout_channel,
      playback_start_timestamp));

  mixer_input_->SetVolumeMultiplier(volume_multiplier_);
  // Create decoder_ if necessary. This can happen if Stop() was called, and
  // SetConfig() was not called since then.
  if (!decoder_) {
    CreateDecoder();
  }
  if (!rate_shifter_) {
    CreateRateShifter(config_.samples_per_second);
  }
  playback_start_timestamp_ = playback_start_timestamp;
  return true;
}

void AudioDecoderForMixer::Stop() {
  TRACE_FUNCTION_ENTRY0();
  decoder_.reset();
  mixer_input_.reset();
  rate_shifter_.reset();
  weak_factory_.InvalidateWeakPtrs();

  Initialize();
}

bool AudioDecoderForMixer::Pause() {
  TRACE_FUNCTION_ENTRY0();
  DCHECK(mixer_input_);
  mixer_input_->SetPaused(true);
  paused_pts_ = GetCurrentPts();
  return true;
}

bool AudioDecoderForMixer::Resume() {
  TRACE_FUNCTION_ENTRY0();
  DCHECK(mixer_input_);
  paused_pts_ = kInvalidTimestamp;
  mixer_input_->SetPaused(false);
  return true;
}

float AudioDecoderForMixer::SetPlaybackRate(float rate) {
  if (std::abs(rate - 1.0) < kPlaybackRateEpsilon) {
    // AudioRendererAlgorithm treats values close to 1 as exactly 1.
    rate = 1.0f;
  }
  LOG(INFO) << "SetPlaybackRate to " << rate;

  // Remove info for rates that have no pending output left.
  while (!rate_shifter_info_.empty()) {
    RateShifterInfo* rate_info = &rate_shifter_info_.back();
    int64_t possible_output_frames = rate_info->input_frames / rate_info->rate;
    DCHECK_GE(possible_output_frames, rate_info->output_frames);
    if (rate_info->output_frames == possible_output_frames) {
      rate_shifter_info_.pop_back();
    } else {
      break;
    }
  }

  rate_shifter_info_.push_back(RateShifterInfo(rate));
  return rate;
}

int64_t AudioDecoderForMixer::GetCurrentPts() const {
  if (paused_pts_ != kInvalidTimestamp)
    return paused_pts_;
  if (last_push_pts_ == kInvalidTimestamp)
    return kInvalidTimestamp;

  DCHECK(!rate_shifter_info_.empty());
  int64_t now = backend_->MonotonicClockNow();
  int64_t estimate =
      last_push_pts_ +
      std::min(static_cast<int64_t>((now - last_push_timestamp_) *
                                    rate_shifter_info_.front().rate),
               last_push_pts_length_);
  return (estimate < first_push_pts_ ? kInvalidTimestamp : estimate);
}

AudioDecoderForMixer::BufferStatus AudioDecoderForMixer::PushBuffer(
    CastDecoderBuffer* buffer) {
  TRACE_FUNCTION_ENTRY0();
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(buffer);
  DCHECK(!got_eos_);
  DCHECK(!mixer_error_);
  DCHECK(!pending_buffer_complete_);

  uint64_t input_bytes = buffer->end_of_stream() ? 0 : buffer->data_size();
  scoped_refptr<DecoderBufferBase> buffer_base(
      static_cast<DecoderBufferBase*>(buffer));

  // If the buffer is already decoded, do not attempt to decode. Call
  // OnBufferDecoded asynchronously on the main thread.
  if (BypassDecoder()) {
    DCHECK(!decoder_);
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&AudioDecoderForMixer::OnBufferDecoded,
                       weak_factory_.GetWeakPtr(), input_bytes,
                       CastAudioDecoder::Status::kDecodeOk, buffer_base));
    return MediaPipelineBackend::kBufferPending;
  }

  DCHECK(decoder_);
  // Decode the buffer.
  decoder_->Decode(buffer_base,
                   base::Bind(&AudioDecoderForMixer::OnBufferDecoded,
                              weak_factory_.GetWeakPtr(), input_bytes));
  return MediaPipelineBackend::kBufferPending;
}

void AudioDecoderForMixer::UpdateStatistics(Statistics delta) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  stats_.decoded_bytes += delta.decoded_bytes;
}

void AudioDecoderForMixer::GetStatistics(Statistics* stats) {
  TRACE_FUNCTION_ENTRY0();
  DCHECK(stats);
  DCHECK(task_runner_->BelongsToCurrentThread());
  *stats = stats_;
}

bool AudioDecoderForMixer::SetConfig(const AudioConfig& config) {
  TRACE_FUNCTION_ENTRY0();
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (!IsValidConfig(config)) {
    LOG(ERROR) << "Invalid audio config passed to SetConfig";
    return false;
  }

  bool changed_sample_rate =
      (config.samples_per_second != config_.samples_per_second);

  if (!rate_shifter_ || changed_sample_rate) {
    CreateRateShifter(config.samples_per_second);
  }

  if (mixer_input_ && changed_sample_rate) {
    // Destroy the old input first to ensure that the mixer output sample rate
    // is updated.
    mixer_input_.reset();
    mixer_input_.reset(new BufferingMixerSource(
        this, config.samples_per_second, backend_->Primary(),
        backend_->DeviceId(), backend_->ContentType(), config.playout_channel,
        playback_start_timestamp_));
    mixer_input_->SetVolumeMultiplier(volume_multiplier_);
    pending_output_frames_ = kNoPendingOutput;
  }

  config_ = config;
  decoder_.reset();
  CreateDecoder();

  if (pending_buffer_complete_ && changed_sample_rate) {
    pending_buffer_complete_ = false;
    delegate_->OnPushBufferComplete(MediaPipelineBackend::kBufferSuccess);
  }
  return true;
}

void AudioDecoderForMixer::CreateDecoder() {
  DCHECK(!decoder_);
  DCHECK(IsValidConfig(config_));

  // No need to create a decoder if the samples are already decoded.
  if (BypassDecoder()) {
    LOG(INFO) << "Data is not coded. Decoder will not be used.";
    return;
  }

  // Create a decoder.
  decoder_ = CastAudioDecoder::Create(
      task_runner_, config_, kDecoderSampleFormat,
      base::Bind(&AudioDecoderForMixer::OnDecoderInitialized,
                 weak_factory_.GetWeakPtr()));
}

void AudioDecoderForMixer::CreateRateShifter(int samples_per_second) {
  rate_shifter_info_.clear();
  rate_shifter_info_.push_back(RateShifterInfo(1.0f));

  rate_shifter_.reset(new ::media::AudioRendererAlgorithm());
  bool is_encrypted = false;
  rate_shifter_->Initialize(
      ::media::AudioParameters(::media::AudioParameters::AUDIO_PCM_LINEAR,
                               ::media::CHANNEL_LAYOUT_STEREO,
                               samples_per_second, kDefaultFramesPerBuffer),
      is_encrypted);
}

bool AudioDecoderForMixer::SetVolume(float multiplier) {
  TRACE_FUNCTION_ENTRY1(multiplier);
  DCHECK(task_runner_->BelongsToCurrentThread());
  volume_multiplier_ = multiplier;
  if (mixer_input_)
    mixer_input_->SetVolumeMultiplier(volume_multiplier_);
  return true;
}

AudioDecoderForMixer::RenderingDelay AudioDecoderForMixer::GetRenderingDelay() {
  TRACE_FUNCTION_ENTRY0();
  AudioDecoderForMixer::RenderingDelay delay = last_mixer_delay_;
  if (delay.timestamp_microseconds != kInvalidTimestamp) {
    double usec_per_sample = 1000000.0 / config_.samples_per_second;

    // Account for data that has been queued in the rate shifters.
    for (const RateShifterInfo& info : rate_shifter_info_) {
      double queued_output_frames =
          (info.input_frames / info.rate) - info.output_frames;
      delay.delay_microseconds += queued_output_frames * usec_per_sample;
    }

    // Account for data that is in the process of being pushed to the mixer.
    if (pending_output_frames_ != kNoPendingOutput) {
      delay.delay_microseconds += pending_output_frames_ * usec_per_sample;
    }
  }

  return delay;
}

void AudioDecoderForMixer::OnDecoderInitialized(bool success) {
  TRACE_FUNCTION_ENTRY0();
  DCHECK(task_runner_->BelongsToCurrentThread());
  LOG(INFO) << "Decoder initialization was "
            << (success ? "successful" : "unsuccessful");
  if (!success)
    delegate_->OnDecoderError();
}

void AudioDecoderForMixer::OnBufferDecoded(
    uint64_t input_bytes,
    CastAudioDecoder::Status status,
    const scoped_refptr<DecoderBufferBase>& decoded) {
  TRACE_FUNCTION_ENTRY0();
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!got_eos_);
  DCHECK(!pending_buffer_complete_);
  DCHECK(rate_shifter_);

  if (status == CastAudioDecoder::Status::kDecodeError) {
    LOG(ERROR) << "Decode error";
    delegate_->OnPushBufferComplete(MediaPipelineBackend::kBufferFailed);
    return;
  }
  if (mixer_error_) {
    delegate_->OnPushBufferComplete(MediaPipelineBackend::kBufferFailed);
    return;
  }

  Statistics delta;
  delta.decoded_bytes = input_bytes;
  UpdateStatistics(delta);

  pending_buffer_complete_ = true;
  if (decoded->end_of_stream()) {
    got_eos_ = true;
  } else {
    int input_frames = decoded->data_size() / (kNumChannels * sizeof(float));

    last_push_pts_ = decoded->timestamp();
    last_push_pts_length_ =
        input_frames * kMicrosecondsPerSecond / config_.samples_per_second;
    if (last_push_pts_ != kInvalidTimestamp) {
      if (first_push_pts_ == kInvalidTimestamp) {
        first_push_pts_ = last_push_pts_;
      }

      RenderingDelay delay = GetRenderingDelay();
      if (delay.timestamp_microseconds == kInvalidTimestamp) {
        last_push_pts_ = kInvalidTimestamp;
      } else {
        last_push_timestamp_ =
            delay.timestamp_microseconds + delay.delay_microseconds;
      }
    }

    DCHECK(!rate_shifter_info_.empty());
    RateShifterInfo* rate_info = &rate_shifter_info_.front();
    // Bypass rate shifter if the rate is 1.0, and there are no frames queued
    // in the rate shifter.
    if (rate_info->rate == 1.0 && rate_shifter_->frames_buffered() == 0 &&
        pending_output_frames_ == kNoPendingOutput &&
        rate_shifter_info_.size() == 1) {
      DCHECK_EQ(rate_info->output_frames, rate_info->input_frames);
      pending_output_frames_ = input_frames;
      if (got_eos_) {
        DCHECK(!pushed_eos_);
        pushed_eos_ = true;
      }
      mixer_input_->WritePcm(decoded);
      return;
    }

    // Otherwise, queue data into the rate shifter, and then try to push the
    // rate-shifted data.
    const uint8_t* channels[kNumChannels] = {
        decoded->data(), decoded->data() + input_frames * sizeof(float)};
    scoped_refptr<::media::AudioBuffer> buffer = ::media::AudioBuffer::CopyFrom(
        ::media::kSampleFormatPlanarF32, ::media::CHANNEL_LAYOUT_STEREO,
        kNumChannels, config_.samples_per_second, input_frames, channels,
        base::TimeDelta(), pool_);
    rate_shifter_->EnqueueBuffer(buffer);
    rate_shifter_info_.back().input_frames += input_frames;
  }

  PushRateShifted();
  DCHECK(!rate_shifter_info_.empty());
  CheckBufferComplete();
}

void AudioDecoderForMixer::CheckBufferComplete() {
  if (!pending_buffer_complete_) {
    return;
  }

  bool rate_shifter_queue_full = rate_shifter_->IsQueueFull();
  DCHECK(!rate_shifter_info_.empty());
  if (rate_shifter_info_.front().rate == 1.0) {
    // If the current rate is 1.0, drain any data in the rate shifter before
    // calling PushBufferComplete, so that the next PushBuffer call can skip the
    // rate shifter entirely.
    rate_shifter_queue_full = (rate_shifter_->frames_buffered() > 0 ||
                               pending_output_frames_ != kNoPendingOutput);
  }

  if (pushed_eos_ || !rate_shifter_queue_full) {
    pending_buffer_complete_ = false;
    delegate_->OnPushBufferComplete(MediaPipelineBackend::kBufferSuccess);
  }
}

void AudioDecoderForMixer::PushRateShifted() {
  DCHECK(mixer_input_);

  if (pushed_eos_ || pending_output_frames_ != kNoPendingOutput) {
    return;
  }

  if (got_eos_) {
    // Push some silence into the rate shifter so we can get out any remaining
    // rate-shifted data.
    rate_shifter_->EnqueueBuffer(::media::AudioBuffer::CreateEmptyBuffer(
        ::media::CHANNEL_LAYOUT_STEREO, kNumChannels,
        config_.samples_per_second, kSilenceBufferFrames, base::TimeDelta()));
  }

  DCHECK(!rate_shifter_info_.empty());
  RateShifterInfo* rate_info = &rate_shifter_info_.front();
  int64_t possible_output_frames = rate_info->input_frames / rate_info->rate;
  DCHECK_GE(possible_output_frames, rate_info->output_frames);

  int desired_output_frames = possible_output_frames - rate_info->output_frames;
  if (desired_output_frames == 0) {
    if (got_eos_) {
      DCHECK(!pushed_eos_);
      pending_output_frames_ = 0;
      pushed_eos_ = true;

      scoped_refptr<DecoderBufferBase> eos_buffer(
          new DecoderBufferAdapter(::media::DecoderBuffer::CreateEOSBuffer()));
      mixer_input_->WritePcm(eos_buffer);
    }
    return;
  }
  // Don't push too many frames at a time.
  desired_output_frames = std::min(
      desired_output_frames,
      config_.samples_per_second * kMaxOutputMs / kMillisecondsPerSecond);

  if (desired_output_frames > rate_shifter_output_->frames()) {
    rate_shifter_output_ =
        ::media::AudioBus::Create(kNumChannels, desired_output_frames);
  }

  int out_frames = rate_shifter_->FillBuffer(
      rate_shifter_output_.get(), 0, desired_output_frames, rate_info->rate);
  if (out_frames <= 0) {
    return;
  }

  rate_info->output_frames += out_frames;
  DCHECK_GE(possible_output_frames, rate_info->output_frames);

  int channel_data_size = out_frames * sizeof(float);
  scoped_refptr<DecoderBufferBase> output_buffer(new DecoderBufferAdapter(
      new ::media::DecoderBuffer(channel_data_size * kNumChannels)));
  for (int c = 0; c < kNumChannels; ++c) {
    memcpy(output_buffer->writable_data() + c * channel_data_size,
           rate_shifter_output_->channel(c), channel_data_size);
  }
  pending_output_frames_ = out_frames;
  mixer_input_->WritePcm(output_buffer);

  if (rate_shifter_info_.size() > 1 &&
      rate_info->output_frames == possible_output_frames) {
    double remaining_input_frames =
        rate_info->input_frames - (rate_info->output_frames * rate_info->rate);
    rate_shifter_info_.pop_front();

    rate_info = &rate_shifter_info_.front();
    LOG(INFO) << "New playback rate in effect: " << rate_info->rate;
    rate_info->input_frames += remaining_input_frames;
    DCHECK_EQ(0, rate_info->output_frames);

    // If new playback rate is 1.0, clear out 'extra' data in the rate shifter.
    // When doing rate shifting, the rate shifter queue holds data after it has
    // been logically played; once we switch to passthrough mode (rate == 1.0),
    // that old data needs to be cleared out.
    if (rate_info->rate == 1.0) {
      int extra_frames = rate_shifter_->frames_buffered() -
                         static_cast<int>(rate_info->input_frames);
      if (extra_frames > 0) {
        // Clear out extra buffered data.
        std::unique_ptr<::media::AudioBus> dropped =
            ::media::AudioBus::Create(kNumChannels, extra_frames);
        int cleared_frames =
            rate_shifter_->FillBuffer(dropped.get(), 0, extra_frames, 1.0f);
        DCHECK_EQ(extra_frames, cleared_frames);
      }
      rate_info->input_frames = rate_shifter_->frames_buffered();
    }
  }
}

bool AudioDecoderForMixer::BypassDecoder() const {
  DCHECK(task_runner_->BelongsToCurrentThread());
  // The mixer input requires planar float PCM data.
  return (config_.codec == kCodecPCM &&
          config_.sample_format == kSampleFormatPlanarF32);
}

void AudioDecoderForMixer::OnWritePcmCompletion(RenderingDelay delay) {
  TRACE_FUNCTION_ENTRY0();
  DCHECK(task_runner_->BelongsToCurrentThread());
  pending_output_frames_ = kNoPendingOutput;
  last_mixer_delay_ = delay;

  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(&AudioDecoderForMixer::PushMorePcm,
                                        weak_factory_.GetWeakPtr()));
}

void AudioDecoderForMixer::PushMorePcm() {
  PushRateShifted();

  DCHECK(!rate_shifter_info_.empty());
  CheckBufferComplete();
}

void AudioDecoderForMixer::OnMixerError(MixerError error) {
  TRACE_FUNCTION_ENTRY0();
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (error != MixerError::kInputIgnored)
    LOG(ERROR) << "Mixer error occurred.";
  mixer_error_ = true;
  delegate_->OnDecoderError();
}

void AudioDecoderForMixer::OnEos() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  delegate_->OnEndOfStream();
}

}  // namespace media
}  // namespace chromecast
