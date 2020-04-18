// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/stream_mixer.h"

#include <pthread.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>
#include <utility>

#include "base/bind_helpers.h"
#include "base/compiler_specific.h"
#include "base/no_destructor.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "chromecast/base/chromecast_switches.h"
#include "chromecast/media/base/audio_device_ids.h"
#include "chromecast/media/cma/backend/cast_audio_json.h"
#include "chromecast/media/cma/backend/filter_group.h"
#include "chromecast/media/cma/backend/post_processing_pipeline_impl.h"
#include "chromecast/media/cma/backend/post_processing_pipeline_parser.h"
#include "chromecast/public/media/audio_post_processor_shlib.h"
#include "chromecast/public/media/mixer_output_stream.h"
#include "media/audio/audio_device_description.h"

#define POST_THROUGH_SHIM_THREAD(method, ...)                                  \
  shim_task_runner_->PostTask(                                                 \
      FROM_HERE, base::BindOnce(&PostTaskShim, mixer_task_runner_,             \
                                base::BindOnce(method, base::Unretained(this), \
                                               ##__VA_ARGS__)));

#define POST_TASK_TO_SHIM_THREAD(method, ...) \
  shim_task_runner_->PostTask(                \
      FROM_HERE,                              \
      base::BindOnce(method, base::Unretained(this), ##__VA_ARGS__));

namespace chromecast {
namespace media {

class StreamMixer::ExternalLoopbackAudioObserver
    : public CastMediaShlib::LoopbackAudioObserver {
 public:
  ExternalLoopbackAudioObserver(StreamMixer* mixer) : mixer_(mixer) {}

  void OnLoopbackAudio(int64_t timestamp,
                       SampleFormat format,
                       int sample_rate,
                       int num_channels,
                       uint8_t* data,
                       int length) override {
    auto loopback_data = std::make_unique<uint8_t[]>(length);
    std::copy(data, data + length, loopback_data.get());
    mixer_->PostLoopbackData(timestamp, format, sample_rate, num_channels,
                             std::move(loopback_data), length);
  }
  void OnLoopbackInterrupted() override { mixer_->PostLoopbackInterrupted(); }

  void OnRemoved() override {
    // We expect that external pipeline will not invoke any other callbacks
    // after this one.
    delete this;
    // No need to pipe this, StreamMixer will let the other observer know when
    // it's being removed.
  }

 private:
  StreamMixer* const mixer_;
};

class StreamMixer::ExternalMediaVolumeChangeRequestObserver
    : public StreamMixer::BaseExternalMediaVolumeChangeRequestObserver {
 public:
  ExternalMediaVolumeChangeRequestObserver(StreamMixer* mixer) : mixer_(mixer) {
    DCHECK(mixer_);
  }

  // ExternalAudioPipelineShlib::ExternalMediaVolumeChangeRequestObserver
  // implementation:
  void OnVolumeChangeRequest(float new_volume) override {
    mixer_->SetVolume(AudioContentType::kMedia, new_volume);
  }

  void OnMuteChangeRequest(bool new_muted) override {
    mixer_->SetVolume(AudioContentType::kMedia, new_muted);
  }

 private:
  StreamMixer* const mixer_;
};

namespace {

const int kNumInputChannels = 2;

const int kDefaultCheckCloseTimeoutMs = 2000;

// Resample all audio below this frequency.
const unsigned int kLowSampleRateCutoff = 32000;

// Sample rate to fall back if input sample rate is below kLowSampleRateCutoff.
const unsigned int kLowSampleRateFallback = 48000;

const int64_t kNoTimestamp = std::numeric_limits<int64_t>::min();

const int kUseDefaultFade = -1;
const int kMediaDuckFadeMs = 150;
const int kMediaUnduckFadeMs = 700;
const int kDefaultFilterFrameAlignment = 64;

void PostTaskShim(scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                  base::OnceClosure task) {
  task_runner->PostTask(FROM_HERE, std::move(task));
}

bool IsOutputDeviceId(const std::string& device) {
  return device == ::media::AudioDeviceDescription::kDefaultDeviceId ||
         device == ::media::AudioDeviceDescription::kCommunicationsDeviceId ||
         device == kLocalAudioDeviceId || device == kAlarmAudioDeviceId ||
         device == kPlatformAudioDeviceId /* e.g. bluetooth and aux */ ||
         device == kTtsAudioDeviceId;
}

std::unique_ptr<FilterGroup> CreateFilterGroup(
    FilterGroup::GroupType type,
    int input_channels,
    bool mix_to_mono,
    const std::string& name,
    const base::ListValue* filter_list,
    const std::unordered_set<std::string>& device_ids,
    const std::vector<FilterGroup*>& mixed_inputs,
    std::unique_ptr<PostProcessingPipelineFactory>& ppp_factory) {
  DCHECK(ppp_factory);
  auto pipeline =
      ppp_factory->CreatePipeline(name, filter_list, input_channels);
  return std::make_unique<FilterGroup>(input_channels, type, mix_to_mono, name,
                                       std::move(pipeline), device_ids,
                                       mixed_inputs);
}

int GetFixedSampleRate() {
  int fixed_sample_rate = GetSwitchValueNonNegativeInt(
      switches::kAudioOutputSampleRate, MixerOutputStream::kInvalidSampleRate);

  if (fixed_sample_rate == MixerOutputStream::kInvalidSampleRate) {
    fixed_sample_rate =
        GetSwitchValueNonNegativeInt(switches::kAlsaFixedOutputSampleRate,
                                     MixerOutputStream::kInvalidSampleRate);
  }
  return fixed_sample_rate;
}

base::TimeDelta GetNoInputCloseTimeout() {
  // --accept-resource-provider should imply a check close timeout of 0.
  int default_close_timeout_ms =
      GetSwitchValueBoolean(switches::kAcceptResourceProvider, false)
          ? 0
          : kDefaultCheckCloseTimeoutMs;
  int close_timeout_ms = GetSwitchValueInt(switches::kAlsaCheckCloseTimeout,
                                           default_close_timeout_ms);
  if (close_timeout_ms < 0) {
    return base::TimeDelta::Max();
  }
  return base::TimeDelta::FromMilliseconds(close_timeout_ms);
}

void UseHighPriority() {
#if (!defined(OS_FUCHSIA) && !defined(OS_ANDROID))
  const struct sched_param kAudioPrio = {10};
  pthread_setschedparam(pthread_self(), SCHED_RR, &kAudioPrio);
#endif
}

}  // namespace

float StreamMixer::VolumeInfo::GetEffectiveVolume() {
  return std::min(volume, limit);
}

// static
StreamMixer* StreamMixer::Get() {
  static base::NoDestructor<StreamMixer> mixer_instance;
  return mixer_instance.get();
}

StreamMixer::StreamMixer()
    : StreamMixer(nullptr,
                  std::make_unique<base::Thread>("CMA mixer"),
                  nullptr) {}

StreamMixer::StreamMixer(
    std::unique_ptr<MixerOutputStream> output,
    std::unique_ptr<base::Thread> mixer_thread,
    scoped_refptr<base::SingleThreadTaskRunner> mixer_task_runner)
    : output_(std::move(output)),
      post_processing_pipeline_factory_(
          std::make_unique<PostProcessingPipelineFactoryImpl>()),
      mixer_thread_(std::move(mixer_thread)),
      mixer_task_runner_(std::move(mixer_task_runner)),
      num_output_channels_(
          GetSwitchValueNonNegativeInt(switches::kAudioOutputChannels,
                                       kNumInputChannels)),
      low_sample_rate_cutoff_(
          GetSwitchValueBoolean(switches::kAlsaEnableUpsampling, false)
              ? kLowSampleRateCutoff
              : MixerOutputStream::kInvalidSampleRate),
      fixed_sample_rate_(GetFixedSampleRate()),
      no_input_close_timeout_(GetNoInputCloseTimeout()),
      filter_frame_alignment_(kDefaultFilterFrameAlignment),
      state_(kStateStopped),
      external_audio_pipeline_supported_(
          ExternalAudioPipelineShlib::IsSupported()),
      weak_factory_(this) {
  VLOG(1) << __func__;

  volume_info_[AudioContentType::kOther].volume = 1.0f;
  volume_info_[AudioContentType::kOther].limit = 1.0f;
  volume_info_[AudioContentType::kOther].muted = false;

  if (mixer_thread_) {
    base::Thread::Options options;
    options.priority = base::ThreadPriority::REALTIME_AUDIO;
    mixer_thread_->StartWithOptions(options);
    mixer_task_runner_ = mixer_thread_->task_runner();
    mixer_task_runner_->PostTask(FROM_HERE, base::BindOnce(&UseHighPriority));

    shim_thread_ = std::make_unique<base::Thread>("CMA mixer PI shim");
    base::Thread::Options shim_options;
    shim_options.priority = base::ThreadPriority::REALTIME_AUDIO;
    shim_thread_->StartWithOptions(shim_options);
    shim_task_runner_ = shim_thread_->task_runner();
    shim_task_runner_->PostTask(FROM_HERE, base::BindOnce(&UseHighPriority));
  } else {
    shim_task_runner_ = mixer_task_runner_;
  }

  if (fixed_sample_rate_ != MixerOutputStream::kInvalidSampleRate) {
    LOG(INFO) << "Setting fixed sample rate to " << fixed_sample_rate_;
  }

  // Read post-processing configuration file.
  PostProcessingPipelineParser pipeline_parser;
  CreatePostProcessors(&pipeline_parser);

  // TODO(jyw): command line flag for filter frame alignment.
  DCHECK_EQ(filter_frame_alignment_ & (filter_frame_alignment_ - 1), 0)
      << "Alignment must be a power of 2.";

  if (external_audio_pipeline_supported_) {
    external_volume_observer_ =
        std::make_unique<ExternalMediaVolumeChangeRequestObserver>(this);
    ExternalAudioPipelineShlib::AddExternalMediaVolumeChangeRequestObserver(
        external_volume_observer_.get());
    external_loopback_audio_observer_ =
        std::make_unique<ExternalLoopbackAudioObserver>(this);
    ExternalAudioPipelineShlib::AddExternalLoopbackAudioObserver(
        external_loopback_audio_observer_.get());
  }
}

void StreamMixer::ResetPostProcessorsForTest(
    std::unique_ptr<PostProcessingPipelineFactory> pipeline_factory,
    const std::string& pipeline_json) {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  LOG(INFO) << __FUNCTION__ << " disregard previous PostProcessor messages.";
  filter_groups_.clear();
  default_filter_ = nullptr;
  post_processing_pipeline_factory_ = std::move(pipeline_factory);
  PostProcessingPipelineParser parser(pipeline_json);
  CreatePostProcessors(&parser);
}

void StreamMixer::SetNumOutputChannelsForTest(int num_output_channels) {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  num_output_channels_ = num_output_channels;
}

void StreamMixer::CreatePostProcessors(
    PostProcessingPipelineParser* pipeline_parser) {
  std::unordered_set<std::string> used_streams;
  for (auto& stream_pipeline : pipeline_parser->GetStreamPipelines()) {
    const auto& device_ids = stream_pipeline.stream_types;
    for (const std::string& stream_type : device_ids) {
      CHECK(IsOutputDeviceId(stream_type))
          << stream_type << " is not a stream type. Stream types are listed "
          << "in chromecast/media/base/audio_device_ids.cc and "
          << "media/audio/audio_device_description.cc";
      CHECK(used_streams.insert(stream_type).second)
          << "Multiple instances of stream type '" << stream_type << "' in "
          << kCastAudioJsonFilePath << ".";
    }
    filter_groups_.push_back(CreateFilterGroup(
        FilterGroup::GroupType::kStream, kNumInputChannels,
        false /* mono_mixer */, *device_ids.begin() /* name */,
        stream_pipeline.pipeline, device_ids,
        std::vector<FilterGroup*>() /* mixed_inputs */,
        post_processing_pipeline_factory_));
    if (device_ids.find(::media::AudioDeviceDescription::kDefaultDeviceId) !=
        device_ids.end()) {
      default_filter_ = filter_groups_.back().get();
    }
  }

  bool enabled_mono_mixer = (num_output_channels_ == 1);
  if (!filter_groups_.empty()) {
    std::vector<FilterGroup*> filter_group_ptrs(filter_groups_.size());
    int mix_group_input_channels = filter_groups_[0]->GetOutputChannelCount();
    for (size_t i = 0; i < filter_groups_.size(); ++i) {
      DCHECK_EQ(mix_group_input_channels,
                filter_groups_[i]->GetOutputChannelCount())
          << "All output stream mixers must have the same number of channels";
      filter_group_ptrs[i] = filter_groups_[i].get();
    }

    // Enable Mono mixer in |mix_filter_| if necessary.
    filter_groups_.push_back(CreateFilterGroup(
        FilterGroup::GroupType::kFinalMix, mix_group_input_channels,
        enabled_mono_mixer, "mix", pipeline_parser->GetMixPipeline(),
        std::unordered_set<std::string>() /* device_ids */, filter_group_ptrs,
        post_processing_pipeline_factory_));
  } else {
    // Mix group directly mixes all inputs.
    std::string kDefaultDeviceId =
        ::media::AudioDeviceDescription::kDefaultDeviceId;
    filter_groups_.push_back(CreateFilterGroup(
        FilterGroup::GroupType::kFinalMix, kNumInputChannels,
        enabled_mono_mixer, "mix", pipeline_parser->GetMixPipeline(),
        std::unordered_set<std::string>({kDefaultDeviceId}),
        std::vector<FilterGroup*>() /* mixed_inputs */,
        post_processing_pipeline_factory_));
    default_filter_ = filter_groups_.back().get();
  }

  mix_filter_ = filter_groups_.back().get();

  filter_groups_.push_back(CreateFilterGroup(
      FilterGroup::GroupType::kLinearize, mix_filter_->GetOutputChannelCount(),
      false /* mono_mixer */, "linearize",
      pipeline_parser->GetLinearizePipeline(),
      std::unordered_set<std::string>() /* device_ids */,
      std::vector<FilterGroup*>({mix_filter_}),
      post_processing_pipeline_factory_));
  linearize_filter_ = filter_groups_.back().get();

  LOG(INFO) << "PostProcessor configuration:";
  if (default_filter_ == mix_filter_) {
    LOG(INFO) << "Stream layer: none";
  } else {
    LOG(INFO) << "Stream layer: " << default_filter_->GetOutputChannelCount()
              << " channels";
  }
  LOG(INFO) << "Mix filter: " << mix_filter_->GetOutputChannelCount()
            << " channels";
  LOG(INFO) << "Linearize filter: "
            << linearize_filter_->GetOutputChannelCount() << " channels";
}

StreamMixer::~StreamMixer() {
  VLOG(1) << __func__;
  if (shim_thread_) {
    shim_thread_->Stop();
  }

  mixer_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&StreamMixer::FinalizeOnMixerThread,
                                base::Unretained(this)));
  if (mixer_thread_) {
    mixer_thread_->Stop();
  }

  if (external_volume_observer_) {
    ExternalAudioPipelineShlib::RemoveExternalLoopbackAudioObserver(
        external_loopback_audio_observer_.get());
    external_loopback_audio_observer_.release();
    ExternalAudioPipelineShlib::RemoveExternalMediaVolumeChangeRequestObserver(
        external_volume_observer_.get());
  }
}

void StreamMixer::FinalizeOnMixerThread() {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  Stop();

  inputs_.clear();
  ignored_inputs_.clear();
}

void StreamMixer::Start() {
  VLOG(1) << __func__;
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  DCHECK(state_ == kStateStopped);
  DCHECK(inputs_.empty());

  if (!output_) {
    if (external_audio_pipeline_supported_) {
      output_ = ExternalAudioPipelineShlib::CreateMixerOutputStream();
    } else {
      output_ = MixerOutputStream::Create();
    }
  }
  DCHECK(output_);

  int requested_sample_rate;
  if (fixed_sample_rate_ != MixerOutputStream::kInvalidSampleRate) {
    requested_sample_rate = fixed_sample_rate_;
  } else if (low_sample_rate_cutoff_ != MixerOutputStream::kInvalidSampleRate &&
             requested_output_samples_per_second_ < low_sample_rate_cutoff_) {
    requested_sample_rate =
        output_samples_per_second_ != MixerOutputStream::kInvalidSampleRate
            ? output_samples_per_second_
            : kLowSampleRateFallback;
  } else {
    requested_sample_rate = requested_output_samples_per_second_;
  }

  if (!output_->Start(requested_sample_rate, num_output_channels_)) {
    Stop();
    return;
  }

  output_samples_per_second_ = output_->GetSampleRate();
  // Make sure the number of frames meets the filter alignment requirements.
  frames_per_write_ =
      output_->OptimalWriteFramesCount() & ~(filter_frame_alignment_ - 1);
  CHECK_GT(frames_per_write_, 0);

  ValidatePostProcessors();

  // Initialize filters.
  for (auto&& filter_group : filter_groups_) {
    filter_group->Initialize(output_samples_per_second_);
  }

  state_ = kStateRunning;

  // Write one buffer of silence to get correct rendering delay in the
  // postprocessors.
  WriteOneBuffer();

  mixer_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&StreamMixer::PlaybackLoop, weak_factory_.GetWeakPtr()));
}

void StreamMixer::Stop() {
  VLOG(1) << __func__;
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());

  weak_factory_.InvalidateWeakPtrs();

  PostLoopbackInterrupted();

  if (output_) {
    output_->Stop();
  }

  state_ = kStateStopped;
  output_samples_per_second_ = MixerOutputStream::kInvalidSampleRate;
}

void StreamMixer::CheckChangeOutputRate(int input_samples_per_second) {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  if (state_ != kStateRunning ||
      input_samples_per_second == requested_output_samples_per_second_ ||
      input_samples_per_second == output_samples_per_second_ ||
      input_samples_per_second < static_cast<int>(low_sample_rate_cutoff_)) {
    return;
  }

  for (const auto& input : inputs_) {
    if (input.second->primary()) {
      return;
    }
  }

  // Ignore existing inputs.
  SignalError(MixerInput::Source::MixerError::kInputIgnored);

  requested_output_samples_per_second_ = input_samples_per_second;

  // Restart the output so that the new output sample rate takes effect.
  Stop();
  Start();
}

void StreamMixer::SignalError(MixerInput::Source::MixerError error) {
  // Move all current inputs to the ignored list and inform them of the error.
  for (auto& input : inputs_) {
    input.second->SignalError(error);
    ignored_inputs_.insert(std::move(input));
  }
  inputs_.clear();
  SetCloseTimeout();
}

void StreamMixer::AddInput(MixerInput::Source* input_source) {
  POST_THROUGH_SHIM_THREAD(&StreamMixer::AddInputOnThread, input_source);
}

void StreamMixer::AddInputOnThread(MixerInput::Source* input_source) {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  DCHECK(input_source);

  LOG(INFO) << "Add input " << input_source;

  // If the new input is a primary one (or there were no inputs previously), we
  // may need to change the output sample rate to match the input sample rate.
  // We only change the output rate if it is not set to a fixed value.
  if ((input_source->primary() || inputs_.empty()) &&
      fixed_sample_rate_ == MixerOutputStream::kInvalidSampleRate) {
    CheckChangeOutputRate(input_source->input_samples_per_second());
  }

  if (state_ == kStateStopped) {
    requested_output_samples_per_second_ =
        input_source->input_samples_per_second();
    Start();
  }

  FilterGroup* input_filter_group = default_filter_;
  for (auto&& filter_group : filter_groups_) {
    if (filter_group->CanProcessInput(input_source->device_id())) {
      input_filter_group = filter_group.get();
      break;
    }
  }
  if (input_filter_group) {
    LOG(INFO) << "Added input of type " << input_source->device_id() << " to "
              << input_filter_group->name();
  } else {
    NOTREACHED() << "Could not find a filter group for "
                 << input_source->device_id() << "\n"
                 << "(consider adding a 'default' processor)";
  }

  auto input = std::make_unique<MixerInput>(
      input_source, output_samples_per_second_, frames_per_write_,
      GetTotalRenderingDelay(input_filter_group), input_filter_group);
  if (state_ != kStateRunning) {
    // Mixer error occurred, signal error.
    MixerInput* input_ptr = input.get();
    ignored_inputs_[input_source] = std::move(input);
    input_ptr->SignalError(MixerInput::Source::MixerError::kInternalError);
    return;
  }

  auto type = input->content_type();
  if (type != AudioContentType::kOther) {
    if (input->primary()) {
      input->SetContentTypeVolume(volume_info_[type].GetEffectiveVolume(),
                                  kUseDefaultFade);
    } else {
      input->SetContentTypeVolume(volume_info_[type].volume, kUseDefaultFade);
    }
    input->SetMuted(volume_info_[type].muted);
  }

  inputs_[input_source] = std::move(input);
  UpdatePlayoutChannel();
}

void StreamMixer::RemoveInput(MixerInput::Source* input_source) {
  POST_THROUGH_SHIM_THREAD(&StreamMixer::RemoveInputOnThread, input_source);
}

void StreamMixer::RemoveInputOnThread(MixerInput::Source* input_source) {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  DCHECK(input_source);

  LOG(INFO) << "Remove input " << input_source;

  inputs_.erase(input_source);
  ignored_inputs_.erase(input_source);
  UpdatePlayoutChannel();

  if (inputs_.empty()) {
    SetCloseTimeout();
  }
}

void StreamMixer::SetCloseTimeout() {
  close_timestamp_ = (no_input_close_timeout_.is_max()
                          ? base::TimeTicks::Max()
                          : base::TimeTicks::Now() + no_input_close_timeout_);
}

void StreamMixer::UpdatePlayoutChannel() {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  DCHECK(mix_filter_);
  DCHECK(linearize_filter_);

  int playout_channel;
  if (inputs_.empty()) {
    playout_channel = kChannelAll;
  } else {
    playout_channel = std::numeric_limits<int>::max();
    for (const auto& it : inputs_) {
      playout_channel =
          std::min(it.second->source()->playout_channel(), playout_channel);
    }
  }

  DCHECK(playout_channel == kChannelAll ||
         playout_channel >= 0 && playout_channel < kNumInputChannels);
  LOG(INFO) << "Update playout channel: " << playout_channel;

  mix_filter_->SetMixToMono(num_output_channels_ == 1 &&
                            playout_channel == kChannelAll);
  for (auto& filter_group : filter_groups_) {
    filter_group->UpdatePlayoutChannel(playout_channel);
  }
}

MediaPipelineBackend::AudioDecoder::RenderingDelay
StreamMixer::GetTotalRenderingDelay(FilterGroup* filter_group) {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  if (!output_) {
    return MediaPipelineBackend::AudioDecoder::RenderingDelay();
  }
  MediaPipelineBackend::AudioDecoder::RenderingDelay delay =
      output_->GetRenderingDelay();
  if (!filter_group) {
    return delay;
  }

  if (filter_group != mix_filter_) {
    delay.delay_microseconds += filter_group->GetRenderingDelayMicroseconds();
  }
  delay.delay_microseconds += mix_filter_->GetRenderingDelayMicroseconds();
  delay.delay_microseconds +=
      linearize_filter_->GetRenderingDelayMicroseconds();

  return delay;
}

void StreamMixer::PlaybackLoop() {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  if (inputs_.empty() && base::TimeTicks::Now() >= close_timestamp_) {
    LOG(INFO) << "Close timeout";
    Stop();
    return;
  }

  WriteOneBuffer();

  mixer_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&StreamMixer::PlaybackLoop, weak_factory_.GetWeakPtr()));
}

void StreamMixer::WriteOneBuffer() {
  // Recursively mix and filter each group.
  MediaPipelineBackend::AudioDecoder::RenderingDelay rendering_delay =
      output_->GetRenderingDelay();
  linearize_filter_->MixAndFilter(frames_per_write_, rendering_delay);

  int64_t expected_playback_time;
  if (rendering_delay.timestamp_microseconds == kNoTimestamp) {
    expected_playback_time = kNoTimestamp;
  } else {
    expected_playback_time = rendering_delay.timestamp_microseconds +
                             rendering_delay.delay_microseconds +
                             linearize_filter_->GetRenderingDelayMicroseconds();
  }
  WriteMixedPcm(frames_per_write_, expected_playback_time);
}

void StreamMixer::WriteMixedPcm(int frames, int64_t expected_playback_time) {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());

  // Downmix reference signal to mono to reduce CPU load.
  int mix_channel_count = mix_filter_->GetOutputChannelCount();
  int loopback_channel_count = mix_channel_count;

  float* mixed_data = mix_filter_->GetOutputBuffer();
  if (num_output_channels_ == 1 && mix_channel_count != 1) {
    for (int i = 0; i < frames; ++i) {
      float sum = 0;
      for (int c = 0; c < mix_channel_count; ++c) {
        sum += mixed_data[i * mix_channel_count + c];
      }
      mixed_data[i] = sum / mix_channel_count;
    }
    loopback_channel_count = 1;
  }

  // Hard limit to [1.0, -1.0]
  for (int i = 0; i < frames * loopback_channel_count; ++i) {
    mixed_data[i] = std::min(1.0f, std::max(-1.0f, mixed_data[i]));
  }

  if (!external_audio_pipeline_supported_) {
    size_t length = frames * loopback_channel_count * sizeof(float);
    auto loopback_data = std::make_unique<uint8_t[]>(length);
    uint8_t* data = reinterpret_cast<uint8_t*>(mixed_data);
    std::copy(data, data + length, loopback_data.get());
    PostLoopbackData(expected_playback_time, kSampleFormatF32,
                     output_samples_per_second_, loopback_channel_count,
                     std::move(loopback_data), length);
  }

  // Drop extra channels from linearize filter if necessary.
  float* linearized_data = linearize_filter_->GetOutputBuffer();
  int linearize_channel_count = linearize_filter_->GetOutputChannelCount();
  if (num_output_channels_ == 1 && linearize_channel_count != 1) {
    for (int i = 0; i < frames; ++i) {
      linearized_data[i] = linearized_data[i * linearize_channel_count];
    }
  }

  // Hard limit to [1.0, -1.0].
  for (int i = 0; i < frames * num_output_channels_; ++i) {
    linearized_data[i] = std::min(1.0f, std::max(-1.0f, linearized_data[i]));
  }

  bool playback_interrupted = false;
  output_->Write(linearized_data, frames * num_output_channels_,
                 &playback_interrupted);

  if (playback_interrupted) {
    PostLoopbackInterrupted();
  }
}

void StreamMixer::AddLoopbackAudioObserver(
    CastMediaShlib::LoopbackAudioObserver* observer) {
  VLOG(1) << __func__;
  POST_TASK_TO_SHIM_THREAD(&StreamMixer::AddLoopbackAudioObserverOnShimThread,
                           observer);
}

void StreamMixer::AddLoopbackAudioObserverOnShimThread(
    CastMediaShlib::LoopbackAudioObserver* observer) {
  VLOG(1) << __func__;
  DCHECK(shim_task_runner_->BelongsToCurrentThread());
  DCHECK(observer);
  loopback_observers_.insert(observer);
}

void StreamMixer::RemoveLoopbackAudioObserver(
    CastMediaShlib::LoopbackAudioObserver* observer) {
  VLOG(1) << __func__;
  POST_TASK_TO_SHIM_THREAD(
      &StreamMixer::RemoveLoopbackAudioObserverOnShimThread, observer);
}

void StreamMixer::RemoveLoopbackAudioObserverOnShimThread(
    CastMediaShlib::LoopbackAudioObserver* observer) {
  VLOG(1) << __func__;
  DCHECK(shim_task_runner_->BelongsToCurrentThread());
  loopback_observers_.erase(observer);
  observer->OnRemoved();
}

void StreamMixer::PostLoopbackData(int64_t expected_playback_time,
                                   SampleFormat format,
                                   int sample_rate,
                                   int channels,
                                   std::unique_ptr<uint8_t[]> data,
                                   int length) {
  POST_TASK_TO_SHIM_THREAD(&StreamMixer::SendLoopbackData,
                           expected_playback_time, format, sample_rate,
                           channels, std::move(data), length);
}

void StreamMixer::SendLoopbackData(int64_t expected_playback_time,
                                   SampleFormat format,
                                   int sample_rate,
                                   int channels,
                                   std::unique_ptr<uint8_t[]> data,
                                   int length) {
  DCHECK(shim_task_runner_->BelongsToCurrentThread());
  for (CastMediaShlib::LoopbackAudioObserver* observer : loopback_observers_) {
    observer->OnLoopbackAudio(expected_playback_time, format, sample_rate,
                              channels, data.get(), length);
  }
}

void StreamMixer::PostLoopbackInterrupted() {
  POST_TASK_TO_SHIM_THREAD(&StreamMixer::LoopbackInterrupted);
}

void StreamMixer::LoopbackInterrupted() {
  DCHECK(shim_task_runner_->BelongsToCurrentThread());
  for (auto* observer : loopback_observers_) {
    observer->OnLoopbackInterrupted();
  }
}

void StreamMixer::SetVolume(AudioContentType type, float level) {
  POST_THROUGH_SHIM_THREAD(&StreamMixer::SetVolumeOnThread, type, level);
}

void StreamMixer::SetVolumeOnThread(AudioContentType type, float level) {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  DCHECK(type != AudioContentType::kOther);

  volume_info_[type].volume = level;
  float effective_volume = volume_info_[type].GetEffectiveVolume();
  for (const auto& input : inputs_) {
    if (input.second->content_type() == type) {
      if (input.second->primary()) {
        input.second->SetContentTypeVolume(effective_volume, kUseDefaultFade);
      } else {
        // Volume limits don't apply to effects streams.
        input.second->SetContentTypeVolume(level, kUseDefaultFade);
      }
    }
  }
  if (external_audio_pipeline_supported_ && type == AudioContentType::kMedia) {
    ExternalAudioPipelineShlib::SetExternalMediaVolume(effective_volume);
  }
}

void StreamMixer::SetMuted(AudioContentType type, bool muted) {
  POST_THROUGH_SHIM_THREAD(&StreamMixer::SetMutedOnThread, type, muted);
}

void StreamMixer::SetMutedOnThread(AudioContentType type, bool muted) {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  DCHECK(type != AudioContentType::kOther);

  volume_info_[type].muted = muted;
  for (const auto& input : inputs_) {
    if (input.second->content_type() == type) {
      input.second->SetMuted(muted);
    }
  }
  if (external_audio_pipeline_supported_ && type == AudioContentType::kMedia) {
    ExternalAudioPipelineShlib::SetExternalMediaMuted(muted);
  }
}

void StreamMixer::SetOutputLimit(AudioContentType type, float limit) {
  POST_THROUGH_SHIM_THREAD(&StreamMixer::SetOutputLimitOnThread, type, limit);
}

void StreamMixer::SetOutputLimitOnThread(AudioContentType type, float limit) {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  DCHECK(type != AudioContentType::kOther);

  LOG(INFO) << "Set volume limit for " << static_cast<int>(type) << " to "
            << limit;
  volume_info_[type].limit = limit;
  float effective_volume = volume_info_[type].GetEffectiveVolume();
  int fade_ms = kUseDefaultFade;
  if (type == AudioContentType::kMedia) {
    if (limit >= 1.0f) {  // Unducking.
      fade_ms = kMediaUnduckFadeMs;
    } else {
      fade_ms = kMediaDuckFadeMs;
    }
  }
  for (const auto& input : inputs_) {
    // Volume limits don't apply to effects streams.
    if (input.second->primary() && input.second->content_type() == type) {
      input.second->SetContentTypeVolume(effective_volume, fade_ms);
    }
  }
  if (external_audio_pipeline_supported_ && type == AudioContentType::kMedia) {
    ExternalAudioPipelineShlib::SetExternalMediaVolume(effective_volume);
  }
}

void StreamMixer::SetVolumeMultiplier(MixerInput::Source* source,
                                      float multiplier) {
  POST_THROUGH_SHIM_THREAD(&StreamMixer::SetVolumeMultiplierOnThread, source,
                           multiplier);
}

void StreamMixer::SetVolumeMultiplierOnThread(MixerInput::Source* source,
                                              float multiplier) {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  auto it = inputs_.find(source);
  if (it != inputs_.end()) {
    it->second->SetVolumeMultiplier(multiplier);
  }
}

void StreamMixer::SetPostProcessorConfig(const std::string& name,
                                         const std::string& config) {
  POST_THROUGH_SHIM_THREAD(&StreamMixer::SetPostProcessorConfigOnThread, name,
                           config);
}

void StreamMixer::SetPostProcessorConfigOnThread(const std::string& name,
                                                 const std::string& config) {
  DCHECK(mixer_task_runner_->BelongsToCurrentThread());
  for (auto&& filter_group : filter_groups_) {
    filter_group->SetPostProcessorConfig(name, config);
  }
}

void StreamMixer::ValidatePostProcessorsForTest() {
  ValidatePostProcessors();
}

void StreamMixer::ValidatePostProcessors() {
  // Ensure filter configuration is viable.
  // This can't be done in CreatePostProcessors() because it breaks tests.
  CHECK(num_output_channels_ == 1 ||
        num_output_channels_ == linearize_filter_->GetOutputChannelCount())
      << "PostProcessor configuration channel count does not match command line"
      << " flag: " << linearize_filter_->GetOutputChannelCount() << " vs "
      << num_output_channels_;
  int loopback_channel_count =
      num_output_channels_ == 1 ? 1 : mix_filter_->GetOutputChannelCount();
  CHECK_LE(loopback_channel_count, 2)
      << "PostProcessor configuration has " << loopback_channel_count
      << " channels after 'mix' group, but only 1 or 2 are allowed.";
}
}  // namespace media
}  // namespace chromecast
