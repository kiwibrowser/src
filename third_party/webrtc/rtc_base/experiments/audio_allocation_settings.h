/*
 *  Copyright 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef RTC_BASE_EXPERIMENTS_AUDIO_ALLOCATION_SETTINGS_H_
#define RTC_BASE_EXPERIMENTS_AUDIO_ALLOCATION_SETTINGS_H_

#include "rtc_base/experiments/field_trial_parser.h"
#include "rtc_base/experiments/field_trial_units.h"
namespace webrtc {
// This class encapsulates the logic that controls how allocation of audio
// bitrate is done. This is primarily based on field trials, but also on the
// values of audio parameters.
class AudioAllocationSettings {
 public:
  AudioAllocationSettings();
  ~AudioAllocationSettings();
  // Returns true if audio feedback should be force disabled.
  bool ForceNoAudioFeedback() const;
  // Returns true if changes in transport sequence number id should be ignored
  // as a trigger for reconfiguration.
  bool IgnoreSeqNumIdChange() const;
  // Returns true if the bitrate allocation range should be configured.
  bool ConfigureRateAllocationRange() const;
  // Returns true if the transport sequence number extension should be enabled.
  bool EnableTransportSequenceNumberExtension() const;
  // Returns true if audio traffic should be included in transport wide feedback
  // packets.
  // |transport_seq_num_extension_header_id| the extension header id for
  // transport sequence numbers. Set to 0 if not the extension is not
  // configured.
  bool IncludeAudioInFeedback(int transport_seq_num_extension_header_id) const;
  // Returns true if target bitrate for audio streams should be updated.
  // |transport_seq_num_extension_header_id| the extension header id for
  // transport sequence numbers. Set to 0 if not the extension is not
  // configured.
  bool UpdateAudioTargetBitrate(
      int transport_seq_num_extension_header_id) const;
  // Returns true if audio should be added to rate allocation when the audio
  // stream is started.
  // |min_bitrate_bps| the configured min bitrate, set to -1 if unset.
  // |max_bitrate_bps| the configured max bitrate, set to -1 if unset.
  // |has_dscp| true is dscp is enabled.
  // |transport_seq_num_extension_header_id| the extension header id for
  // transport sequence numbers. Set to 0 if not the extension is not
  // configured.
  bool IncludeAudioInAllocationOnStart(
      int min_bitrate_bps,
      int max_bitrate_bps,
      bool has_dscp,
      int transport_seq_num_extension_header_id) const;
  // Returns true if audio should be added to rate allocation when the audio
  // stream is reconfigured.
  // |min_bitrate_bps| the configured min bitrate, set to -1 if unset.
  // |max_bitrate_bps| the configured max bitrate, set to -1 if unset.
  // |has_dscp| true is dscp is enabled.
  // |transport_seq_num_extension_header_id| the extension header id for
  // transport sequence numbers. Set to 0 if not the extension is not
  // configured.
  bool IncludeAudioInAllocationOnReconfigure(
      int min_bitrate_bps,
      int max_bitrate_bps,
      bool has_dscp,
      int transport_seq_num_extension_header_id) const;

  // Returns the min bitrate for audio rate allocation, potentially including
  // overhead.
  int MinBitrateBps() const;
  // Returns the max bitrate for audio rate allocation, potentially including
  // overhead. |rtp_parameter_max_bitrate_bps| max bitrate as configured in rtp
  // parameters, excluding overhead.
  int MaxBitrateBps(absl::optional<int> rtp_parameter_max_bitrate_bps) const;

 private:
  FieldTrialFlag audio_send_side_bwe_;
  FieldTrialFlag allocate_audio_without_feedback_;
  FieldTrialFlag force_no_audio_feedback_;
  FieldTrialFlag audio_feedback_to_improve_video_bwe_;
  FieldTrialFlag send_side_bwe_with_overhead_;
  int min_overhead_bps_ = 0;
};
}  // namespace webrtc

#endif  // RTC_BASE_EXPERIMENTS_AUDIO_ALLOCATION_SETTINGS_H_
