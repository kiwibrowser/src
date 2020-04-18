// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/media_stream_audio_processor_options.h"

#include <stddef.h>
#include <utility>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "content/public/common/content_features.h"
#include "content/renderer/media/stream/media_stream_constraints_util.h"
#include "content/renderer/media/stream/media_stream_source.h"
#include "media/base/audio_parameters.h"
#include "third_party/webrtc/modules/audio_processing/aec_dump/aec_dump_factory.h"
#include "third_party/webrtc/modules/audio_processing/include/audio_processing.h"
#include "third_party/webrtc/modules/audio_processing/typing_detection.h"

namespace content {

namespace {

// Used to log echo quality based on delay estimates.
enum DelayBasedEchoQuality {
  DELAY_BASED_ECHO_QUALITY_GOOD = 0,
  DELAY_BASED_ECHO_QUALITY_SPURIOUS,
  DELAY_BASED_ECHO_QUALITY_BAD,
  DELAY_BASED_ECHO_QUALITY_INVALID,
  DELAY_BASED_ECHO_QUALITY_MAX
};

DelayBasedEchoQuality EchoDelayFrequencyToQuality(float delay_frequency) {
  const float kEchoDelayFrequencyLowerLimit = 0.1f;
  const float kEchoDelayFrequencyUpperLimit = 0.8f;
  // DELAY_BASED_ECHO_QUALITY_GOOD
  //   delay is out of bounds during at most 10 % of the time.
  // DELAY_BASED_ECHO_QUALITY_SPURIOUS
  //   delay is out of bounds 10-80 % of the time.
  // DELAY_BASED_ECHO_QUALITY_BAD
  //   delay is mostly out of bounds >= 80 % of the time.
  // DELAY_BASED_ECHO_QUALITY_INVALID
  //   delay_frequency is negative which happens if we have insufficient data.
  if (delay_frequency < 0)
    return DELAY_BASED_ECHO_QUALITY_INVALID;
  else if (delay_frequency <= kEchoDelayFrequencyLowerLimit)
    return DELAY_BASED_ECHO_QUALITY_GOOD;
  else if (delay_frequency < kEchoDelayFrequencyUpperLimit)
    return DELAY_BASED_ECHO_QUALITY_SPURIOUS;
  else
    return DELAY_BASED_ECHO_QUALITY_BAD;
}

}  // namespace

AudioProcessingProperties::AudioProcessingProperties() = default;
AudioProcessingProperties::AudioProcessingProperties(
    const AudioProcessingProperties& other) = default;
AudioProcessingProperties& AudioProcessingProperties::operator=(
    const AudioProcessingProperties& other) = default;
AudioProcessingProperties::AudioProcessingProperties(
    AudioProcessingProperties&& other) = default;
AudioProcessingProperties& AudioProcessingProperties::operator=(
    AudioProcessingProperties&& other) = default;
AudioProcessingProperties::~AudioProcessingProperties() = default;

void AudioProcessingProperties::DisableDefaultProperties() {
  enable_sw_echo_cancellation = false;
  disable_hw_echo_cancellation = false;
  goog_auto_gain_control = false;
  goog_experimental_echo_cancellation = false;
  goog_typing_noise_detection = false;
  goog_noise_suppression = false;
  goog_experimental_noise_suppression = false;
  goog_beamforming = false;
  goog_highpass_filter = false;
  goog_experimental_auto_gain_control = false;
}

EchoInformation::EchoInformation()
    : delay_stats_time_ms_(0),
      echo_frames_received_(false),
      divergent_filter_stats_time_ms_(0),
      num_divergent_filter_fraction_(0),
      num_non_zero_divergent_filter_fraction_(0) {}

EchoInformation::~EchoInformation() {
  DCHECK(thread_checker_.CalledOnValidThread());
  ReportAndResetAecDivergentFilterStats();
}

void EchoInformation::UpdateAecStats(
    webrtc::EchoCancellation* echo_cancellation) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!echo_cancellation->is_enabled())
    return;

  UpdateAecDelayStats(echo_cancellation);
  UpdateAecDivergentFilterStats(echo_cancellation);
}

void EchoInformation::UpdateAecDelayStats(
    webrtc::EchoCancellation* echo_cancellation) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Only start collecting stats if we know echo cancellation has measured an
  // echo. Otherwise we clutter the stats with for example cases where only the
  // microphone is used.
  if (!echo_frames_received_ & !echo_cancellation->stream_has_echo())
    return;

  echo_frames_received_ = true;

  // In WebRTC, three echo delay metrics are calculated and updated every
  // five seconds. We use one of them, |fraction_poor_delays| to log in a UMA
  // histogram an Echo Cancellation quality metric. The stat in WebRTC has a
  // fixed aggregation window of five seconds, so we use the same query
  // frequency to avoid logging old values.
  if (!echo_cancellation->is_delay_logging_enabled())
    return;

  delay_stats_time_ms_ += webrtc::AudioProcessing::kChunkSizeMs;
  if (delay_stats_time_ms_ <
      500 * webrtc::AudioProcessing::kChunkSizeMs) {  // 5 seconds
    return;
  }

  int dummy_median = 0, dummy_std = 0;
  float fraction_poor_delays = 0;
  if (echo_cancellation->GetDelayMetrics(
          &dummy_median, &dummy_std, &fraction_poor_delays) ==
      webrtc::AudioProcessing::kNoError) {
    delay_stats_time_ms_ = 0;
    // Map |fraction_poor_delays| to an Echo Cancellation quality and log in UMA
    // histogram. See DelayBasedEchoQuality for information on histogram
    // buckets.
    UMA_HISTOGRAM_ENUMERATION("WebRTC.AecDelayBasedQuality",
                              EchoDelayFrequencyToQuality(fraction_poor_delays),
                              DELAY_BASED_ECHO_QUALITY_MAX);
  }
}

void EchoInformation::UpdateAecDivergentFilterStats(
    webrtc::EchoCancellation* echo_cancellation) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!echo_cancellation->are_metrics_enabled())
    return;

  divergent_filter_stats_time_ms_ += webrtc::AudioProcessing::kChunkSizeMs;
  if (divergent_filter_stats_time_ms_ <
      100 * webrtc::AudioProcessing::kChunkSizeMs) {  // 1 second
    return;
  }

  webrtc::EchoCancellation::Metrics metrics;
  if (echo_cancellation->GetMetrics(&metrics) ==
      webrtc::AudioProcessing::kNoError) {
    // If not yet calculated, |metrics.divergent_filter_fraction| is -1.0. After
    // being calculated the first time, it is updated periodically.
    if (metrics.divergent_filter_fraction < 0.0f) {
      DCHECK_EQ(num_divergent_filter_fraction_, 0);
      return;
    }
    if (metrics.divergent_filter_fraction > 0.0f) {
      ++num_non_zero_divergent_filter_fraction_;
    }
  } else {
    DLOG(WARNING) << "Get echo cancellation metrics failed.";
  }
  ++num_divergent_filter_fraction_;
  divergent_filter_stats_time_ms_ = 0;
}

void EchoInformation::ReportAndResetAecDivergentFilterStats() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (num_divergent_filter_fraction_ == 0)
    return;

  int non_zero_percent = 100 * num_non_zero_divergent_filter_fraction_ /
                         num_divergent_filter_fraction_;
  UMA_HISTOGRAM_PERCENTAGE("WebRTC.AecFilterHasDivergence", non_zero_percent);

  divergent_filter_stats_time_ms_ = 0;
  num_non_zero_divergent_filter_fraction_ = 0;
  num_divergent_filter_fraction_ = 0;
}

void EnableEchoCancellation(AudioProcessing* audio_processing) {
#if defined(OS_ANDROID)
  // Mobile devices are using AECM.
  CHECK_EQ(0, audio_processing->echo_control_mobile()->set_routing_mode(
                  webrtc::EchoControlMobile::kSpeakerphone));
  CHECK_EQ(0, audio_processing->echo_control_mobile()->Enable(true));
  return;
#endif
  int err = audio_processing->echo_cancellation()->set_suppression_level(
      webrtc::EchoCancellation::kHighSuppression);

  // Enable the metrics for AEC.
  err |= audio_processing->echo_cancellation()->enable_metrics(true);
  err |= audio_processing->echo_cancellation()->enable_delay_logging(true);
  err |= audio_processing->echo_cancellation()->Enable(true);
  CHECK_EQ(err, 0);
}

void EnableNoiseSuppression(AudioProcessing* audio_processing,
                            webrtc::NoiseSuppression::Level ns_level) {
  int err = audio_processing->noise_suppression()->set_level(ns_level);
  err |= audio_processing->noise_suppression()->Enable(true);
  CHECK_EQ(err, 0);
}

void EnableTypingDetection(AudioProcessing* audio_processing,
                           webrtc::TypingDetection* typing_detector) {
  int err = audio_processing->voice_detection()->Enable(true);
  err |= audio_processing->voice_detection()->set_likelihood(
      webrtc::VoiceDetection::kVeryLowLikelihood);
  CHECK_EQ(err, 0);

  // Configure the update period to 1s (100 * 10ms) in the typing detector.
  typing_detector->SetParameters(0, 0, 0, 0, 0, 100);
}

void StartEchoCancellationDump(AudioProcessing* audio_processing,
                               base::File aec_dump_file,
                               rtc::TaskQueue* worker_queue) {
  DCHECK(aec_dump_file.IsValid());

  FILE* stream = base::FileToFILE(std::move(aec_dump_file), "w");
  if (!stream) {
    LOG(DFATAL) << "Failed to open AEC dump file";
    return;
  }

  auto aec_dump = webrtc::AecDumpFactory::Create(
      stream, -1 /* max_log_size_bytes */, worker_queue);
  if (!aec_dump) {
    LOG(ERROR) << "Failed to start AEC debug recording";
    return;
  }
  audio_processing->AttachAecDump(std::move(aec_dump));
}

void StopEchoCancellationDump(AudioProcessing* audio_processing) {
  audio_processing->DetachAecDump();
}

void EnableAutomaticGainControl(AudioProcessing* audio_processing) {
#if defined(OS_ANDROID)
  const webrtc::GainControl::Mode mode = webrtc::GainControl::kFixedDigital;
#else
  const webrtc::GainControl::Mode mode = webrtc::GainControl::kAdaptiveAnalog;
#endif
  int err = audio_processing->gain_control()->set_mode(mode);
  err |= audio_processing->gain_control()->Enable(true);
  CHECK_EQ(err, 0);
}

void GetAudioProcessingStats(
    AudioProcessing* audio_processing,
    webrtc::AudioProcessorInterface::AudioProcessorStats* stats) {
  // TODO(ivoc): Change the APM stats to use rtc::Optional instead of default
  //             values.
  auto apm_stats = audio_processing->GetStatistics();
  stats->echo_return_loss = apm_stats.echo_return_loss.instant();
  stats->echo_return_loss_enhancement =
      apm_stats.echo_return_loss_enhancement.instant();
  stats->aec_divergent_filter_fraction = apm_stats.divergent_filter_fraction;

  stats->echo_delay_median_ms = apm_stats.delay_median;
  stats->echo_delay_std_ms = apm_stats.delay_standard_deviation;

  stats->residual_echo_likelihood = apm_stats.residual_echo_likelihood;
  stats->residual_echo_likelihood_recent_max =
      apm_stats.residual_echo_likelihood_recent_max;
}

}  // namespace content
