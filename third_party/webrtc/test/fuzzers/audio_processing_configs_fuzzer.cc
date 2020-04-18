/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "api/audio/echo_canceller3_factory.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "rtc_base/numerics/safe_minmax.h"
#include "test/fuzzers/audio_processing_fuzzer_helper.h"
#include "test/fuzzers/fuzz_data_helper.h"

namespace webrtc {
namespace {
std::unique_ptr<AudioProcessing> CreateApm(test::FuzzDataHelper* fuzz_data) {
  // Parse boolean values for optionally enabling different
  // configurable public components of APM.
  bool exp_agc = fuzz_data->ReadOrDefaultValue(true);
  bool exp_ns = fuzz_data->ReadOrDefaultValue(true);
  bool bf = fuzz_data->ReadOrDefaultValue(true);
  bool ef = fuzz_data->ReadOrDefaultValue(true);
  bool raf = fuzz_data->ReadOrDefaultValue(true);
  bool da = fuzz_data->ReadOrDefaultValue(true);
  bool ie = fuzz_data->ReadOrDefaultValue(true);
  bool red = fuzz_data->ReadOrDefaultValue(true);
  bool hpf = fuzz_data->ReadOrDefaultValue(true);
  bool aec3 = fuzz_data->ReadOrDefaultValue(true);

  bool use_aec = fuzz_data->ReadOrDefaultValue(true);
  bool use_aecm = fuzz_data->ReadOrDefaultValue(true);
  bool use_agc = fuzz_data->ReadOrDefaultValue(true);
  bool use_ns = fuzz_data->ReadOrDefaultValue(true);
  bool use_le = fuzz_data->ReadOrDefaultValue(true);
  bool use_vad = fuzz_data->ReadOrDefaultValue(true);
  bool use_agc_limiter = fuzz_data->ReadOrDefaultValue(true);
  bool use_agc2_limiter = fuzz_data->ReadOrDefaultValue(true);

  // Read an int8 value, but don't let it be too large or small.
  const float gain_controller2_gain_db =
      rtc::SafeClamp<int>(fuzz_data->ReadOrDefaultValue<int8_t>(0), -50, 50);

  // Ignore a few bytes. Bytes from this segment will be used for
  // future config flag changes. We assume 40 bytes is enough for
  // configuring the APM.
  constexpr size_t kSizeOfConfigSegment = 40;
  RTC_DCHECK(kSizeOfConfigSegment >= fuzz_data->BytesRead());
  static_cast<void>(
      fuzz_data->ReadByteArray(kSizeOfConfigSegment - fuzz_data->BytesRead()));

  // Filter out incompatible settings that lead to CHECK failures.
  if (use_aecm && use_aec) {
    return nullptr;
  }

  // Components can be enabled through webrtc::Config and
  // webrtc::AudioProcessingConfig.
  Config config;

  std::unique_ptr<EchoControlFactory> echo_control_factory;
  if (aec3) {
    echo_control_factory.reset(new EchoCanceller3Factory());
  }

  config.Set<ExperimentalAgc>(new ExperimentalAgc(exp_agc));
  config.Set<ExperimentalNs>(new ExperimentalNs(exp_ns));
  if (bf) {
    config.Set<Beamforming>(new Beamforming());
  }
  config.Set<ExtendedFilter>(new ExtendedFilter(ef));
  config.Set<RefinedAdaptiveFilter>(new RefinedAdaptiveFilter(raf));
  config.Set<DelayAgnostic>(new DelayAgnostic(da));
  config.Set<Intelligibility>(new Intelligibility(ie));

  std::unique_ptr<AudioProcessing> apm(
      AudioProcessingBuilder()
          .SetEchoControlFactory(std::move(echo_control_factory))
          .Create(config));

  webrtc::AudioProcessing::Config apm_config;
  apm_config.residual_echo_detector.enabled = red;
  apm_config.high_pass_filter.enabled = hpf;
  apm_config.gain_controller2.enabled = use_agc2_limiter;

  apm_config.gain_controller2.fixed_gain_db = gain_controller2_gain_db;

  apm->ApplyConfig(apm_config);

  apm->echo_cancellation()->Enable(use_aec);
  apm->echo_control_mobile()->Enable(use_aecm);
  apm->gain_control()->Enable(use_agc);
  apm->noise_suppression()->Enable(use_ns);
  apm->level_estimator()->Enable(use_le);
  apm->voice_detection()->Enable(use_vad);
  apm->gain_control()->enable_limiter(use_agc_limiter);

  return apm;
}
}  // namespace

void FuzzOneInput(const uint8_t* data, size_t size) {
  test::FuzzDataHelper fuzz_data(rtc::ArrayView<const uint8_t>(data, size));
  auto apm = CreateApm(&fuzz_data);

  if (apm) {
    FuzzAudioProcessing(&fuzz_data, std::move(apm));
  }
}
}  // namespace webrtc
