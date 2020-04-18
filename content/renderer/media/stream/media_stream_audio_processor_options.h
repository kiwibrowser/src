// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_AUDIO_PROCESSOR_OPTIONS_H_
#define CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_AUDIO_PROCESSOR_OPTIONS_H_

#include <string>
#include <vector>

#include "base/files/file.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "content/common/content_export.h"
#include "content/public/common/media_stream_request.h"
#include "media/base/audio_point.h"
#include "third_party/blink/public/platform/web_media_constraints.h"
#include "third_party/webrtc/api/mediastreaminterface.h"
#include "third_party/webrtc/media/base/mediachannel.h"
#include "third_party/webrtc/modules/audio_processing/include/audio_processing.h"
#include "third_party/webrtc/rtc_base/task_queue.h"

namespace webrtc {

class EchoCancellation;
class TypingDetection;

}

namespace content {

using webrtc::AudioProcessing;

// Simple struct with audio-processing properties.
struct CONTENT_EXPORT AudioProcessingProperties {
  // Creates an AudioProcessingProperties object with fields initialized to
  // their default values.
  AudioProcessingProperties();
  AudioProcessingProperties(const AudioProcessingProperties& other);
  AudioProcessingProperties& operator=(const AudioProcessingProperties& other);
  AudioProcessingProperties(AudioProcessingProperties&& other);
  AudioProcessingProperties& operator=(AudioProcessingProperties&& other);
  ~AudioProcessingProperties();

  // Disables properties that are enabled by default.
  void DisableDefaultProperties();

  bool enable_sw_echo_cancellation = true;
  bool disable_hw_echo_cancellation = true;
  bool enable_experimental_hw_echo_cancellation = false;
  bool disable_hw_noise_suppression = false;
  bool goog_audio_mirroring = false;
  bool goog_auto_gain_control = true;
  bool goog_experimental_echo_cancellation =
#if defined(OS_ANDROID)
      false;
#else
      true;
#endif
  bool goog_typing_noise_detection = true;
  bool goog_noise_suppression = true;
  bool goog_experimental_noise_suppression = true;
  bool goog_beamforming = true;
  bool goog_highpass_filter = true;
  bool goog_experimental_auto_gain_control = true;
  std::vector<media::Point> goog_array_geometry;
};

// A helper class to log echo information in general and Echo Cancellation
// quality in particular.
class CONTENT_EXPORT EchoInformation {
 public:
  EchoInformation();
  virtual ~EchoInformation();

  // Updates stats, and reports delay metrics as UMA stats every 5 seconds.
  // Must be called every time AudioProcessing::ProcessStream() is called.
  void UpdateAecStats(webrtc::EchoCancellation* echo_cancellation);

  // Reports AEC divergent filter metrics as UMA and resets the associated data.
  void ReportAndResetAecDivergentFilterStats();

 private:
  void UpdateAecDelayStats(webrtc::EchoCancellation* echo_cancellation);
  void UpdateAecDivergentFilterStats(
      webrtc::EchoCancellation* echo_cancellation);

  // Counter to track 5 seconds of data in order to query a new metric from
  // webrtc::EchoCancellation::GetEchoDelayMetrics().
  int delay_stats_time_ms_;
  bool echo_frames_received_;

  // Counter to track 1 second of data in order to query a new divergent filter
  // fraction metric from webrtc::EchoCancellation::GetMetrics().
  int divergent_filter_stats_time_ms_;

  // Total number of times we queried for the divergent filter fraction metric.
  int num_divergent_filter_fraction_;

  // Number of non-zero divergent filter fraction metrics.
  int num_non_zero_divergent_filter_fraction_;

  // Ensures that this class is accessed on the same thread.
  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(EchoInformation);
};

// Enables the echo cancellation in |audio_processing|.
void EnableEchoCancellation(AudioProcessing* audio_processing);

// Enables the noise suppression in |audio_processing|.
void EnableNoiseSuppression(AudioProcessing* audio_processing,
                            webrtc::NoiseSuppression::Level ns_level);

// Enables the typing detection in |audio_processing|.
void EnableTypingDetection(AudioProcessing* audio_processing,
                           webrtc::TypingDetection* typing_detector);

// Starts the echo cancellation dump in
// |audio_processing|. |worker_queue| must be kept alive until either
// |audio_processing| is destroyed, or
// StopEchoCancellationDump(audio_processing) is called.
void StartEchoCancellationDump(AudioProcessing* audio_processing,
                               base::File aec_dump_file,
                               rtc::TaskQueue* worker_queue);

// Stops the echo cancellation dump in |audio_processing|.
// This method has no impact if echo cancellation dump has not been started on
// |audio_processing|.
void StopEchoCancellationDump(AudioProcessing* audio_processing);

void EnableAutomaticGainControl(AudioProcessing* audio_processing);

void GetAudioProcessingStats(
    AudioProcessing* audio_processing,
    webrtc::AudioProcessorInterface::AudioProcessorStats* stats);

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_AUDIO_PROCESSOR_OPTIONS_H_
