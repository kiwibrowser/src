// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/media_stream_constraints_util_audio.h"

#include <cmath>
#include <memory>
#include <string>
#include <utility>

#include "base/message_loop/message_loop.h"
#include "content/common/media/media_stream_controls.h"
#include "content/renderer/media/stream/local_media_stream_audio_source.h"
#include "content/renderer/media/stream/media_stream_audio_source.h"
#include "content/renderer/media/stream/media_stream_source.h"
#include "content/renderer/media/stream/mock_constraint_factory.h"
#include "content/renderer/media/stream/processed_local_audio_source.h"
#include "content/renderer/media/webrtc/mock_peer_connection_dependency_factory.h"
#include "media/base/audio_parameters.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/mediastream/media_devices.mojom.h"
#include "third_party/blink/public/platform/web_media_constraints.h"
#include "third_party/blink/public/platform/web_string.h"

namespace content {

namespace {

using BoolSetFunction = void (blink::BooleanConstraint::*)(bool);
using StringSetFunction =
    void (blink::StringConstraint::*)(const blink::WebString&);
using MockFactoryAccessor =
    blink::WebMediaTrackConstraintSet& (MockConstraintFactory::*)();

const BoolSetFunction kBoolSetFunctions[] = {
    &blink::BooleanConstraint::SetExact, &blink::BooleanConstraint::SetIdeal,
};

const StringSetFunction kStringSetFunctions[] = {
    &blink::StringConstraint::SetExact, &blink::StringConstraint::SetIdeal,
};

const MockFactoryAccessor kFactoryAccessors[] = {
    &MockConstraintFactory::basic, &MockConstraintFactory::AddAdvanced};

const bool kBoolValues[] = {true, false};

using AudioSettingsBoolMembers =
    std::vector<bool (AudioCaptureSettings::*)() const>;
using AudioPropertiesBoolMembers =
    std::vector<bool AudioProcessingProperties::*>;

template <typename T>
static bool Contains(const std::vector<T>& vector, T value) {
  auto it = std::find(vector.begin(), vector.end(), value);
  return it != vector.end();
}

}  // namespace

class MediaStreamConstraintsUtilAudioTest
    : public testing::TestWithParam<std::string> {
 public:
  void SetUp() override {
    ResetFactory();
    if (IsDeviceCapture()) {
      capabilities_.emplace_back(
          "default_device",
          media::AudioParameters(media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
                                 media::CHANNEL_LAYOUT_STEREO,
                                 media::AudioParameters::kAudioCDSampleRate,
                                 1000));

      media::AudioParameters hw_echo_canceller_parameters(
          media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
          media::CHANNEL_LAYOUT_STEREO,
          media::AudioParameters::kAudioCDSampleRate, 1000);
      hw_echo_canceller_parameters.set_effects(
          media::AudioParameters::ECHO_CANCELLER);
      capabilities_.emplace_back("hw_echo_canceller_device",
                                 hw_echo_canceller_parameters);

      media::AudioParameters geometry_parameters(
          media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
          media::CHANNEL_LAYOUT_STEREO,
          media::AudioParameters::kAudioCDSampleRate, 1000);
      geometry_parameters.set_mic_positions(kMicPositions);
      capabilities_.emplace_back("geometry device", geometry_parameters);

      default_device_ = &capabilities_[0];
      hw_echo_canceller_device_ = &capabilities_[1];
      geometry_device_ = &capabilities_[2];
    } else {
      // For content capture, use a single capability that admits all possible
      // settings.
      capabilities_.emplace_back();
    }
  }

 protected:
  void MakeHardwareEchoCancellerDeviceExperimental() {
    media::AudioParameters experimental_hw_echo_canceller_parameters(
        media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
        media::CHANNEL_LAYOUT_STEREO,
        media::AudioParameters::kAudioCDSampleRate, 1000);
    experimental_hw_echo_canceller_parameters.set_effects(
        media::AudioParameters::EXPERIMENTAL_ECHO_CANCELLER);
    capabilities_[1] = {"experimental_hw_echo_canceller_device",
                        experimental_hw_echo_canceller_parameters};
  }

  void SetMediaStreamSource(const std::string& source) {}

  void ResetFactory() {
    constraint_factory_.Reset();
    constraint_factory_.basic().media_stream_source.SetExact(
        blink::WebString::FromASCII(GetParam()));
  }

  std::string GetMediaStreamSource() { return GetParam(); }
  bool IsDeviceCapture() { return GetMediaStreamSource().empty(); }

  MediaStreamType GetMediaStreamType() {
    std::string media_source = GetMediaStreamSource();
    if (media_source.empty())
      return MEDIA_DEVICE_AUDIO_CAPTURE;
    else if (media_source == kMediaStreamSourceTab)
      return MEDIA_TAB_AUDIO_CAPTURE;
    return MEDIA_DESKTOP_AUDIO_CAPTURE;
  }

  std::unique_ptr<ProcessedLocalAudioSource> GetProcessedLocalAudioSource(
      const AudioProcessingProperties& properties,
      bool hotword_enabled,
      bool disable_local_echo,
      bool render_to_associated_sink,
      int effects) {
    MediaStreamDevice device;
    device.id = "processed_source";
    device.type = GetMediaStreamType();
    if (render_to_associated_sink)
      device.matched_output_device_id = std::string("some_device_id");
    device.input.set_effects(effects);

    return std::make_unique<ProcessedLocalAudioSource>(
        -1, device, hotword_enabled, disable_local_echo, properties,
        MediaStreamSource::ConstraintsCallback(), &pc_factory_);
  }

  std::unique_ptr<ProcessedLocalAudioSource> GetProcessedLocalAudioSource(
      const AudioProcessingProperties& properties,
      bool hotword_enabled,
      bool disable_local_echo,
      bool render_to_associated_sink) {
    return GetProcessedLocalAudioSource(
        properties, hotword_enabled, disable_local_echo,
        render_to_associated_sink,
        media::AudioParameters::PlatformEffectsMask::NO_EFFECTS);
  }

  std::unique_ptr<LocalMediaStreamAudioSource> GetLocalMediaStreamAudioSource(
      bool enable_hardware_echo_canceller,
      bool hotword_enabled,
      bool disable_local_echo,
      bool render_to_associated_sink) {
    MediaStreamDevice device;
    device.type = GetMediaStreamType();
    if (enable_hardware_echo_canceller)
      device.input.set_effects(media::AudioParameters::ECHO_CANCELLER);
    if (render_to_associated_sink)
      device.matched_output_device_id = std::string("some_device_id");

    return std::make_unique<LocalMediaStreamAudioSource>(
        -1, device, hotword_enabled, disable_local_echo,
        MediaStreamSource::ConstraintsCallback());
  }

  AudioCaptureSettings SelectSettings() {
    blink::WebMediaConstraints constraints =
        constraint_factory_.CreateWebMediaConstraints();
    return SelectSettingsAudioCapture(capabilities_, constraints, false);
  }

  // When googExperimentalEchoCancellation is not explicitly set, its default
  // value is always false on Android. On other platforms it behaves like other
  // audio-processing properties.
  void CheckGoogExperimentalEchoCancellationDefault(
      const AudioProcessingProperties& properties,
      bool value) {
#if defined(OS_ANDROID)
    EXPECT_FALSE(properties.goog_experimental_echo_cancellation);
#else
    EXPECT_EQ(value, properties.goog_experimental_echo_cancellation);
#endif
  }

  void CheckBoolDefaultsDeviceCapture(
      const AudioSettingsBoolMembers& exclude_main_settings,
      const AudioPropertiesBoolMembers& exclude_audio_properties,
      const AudioCaptureSettings& result) {
    if (!Contains(exclude_main_settings,
                  &AudioCaptureSettings::hotword_enabled)) {
      EXPECT_FALSE(result.hotword_enabled());
    }
    if (!Contains(exclude_main_settings,
                  &AudioCaptureSettings::disable_local_echo)) {
      EXPECT_TRUE(result.disable_local_echo());
    }
    if (!Contains(exclude_main_settings,
                  &AudioCaptureSettings::render_to_associated_sink)) {
      EXPECT_FALSE(result.render_to_associated_sink());
    }

    const auto& properties = result.audio_processing_properties();
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::enable_sw_echo_cancellation)) {
      EXPECT_TRUE(properties.enable_sw_echo_cancellation);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::disable_hw_echo_cancellation)) {
      EXPECT_TRUE(properties.disable_hw_echo_cancellation);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::
                      enable_experimental_hw_echo_cancellation)) {
      EXPECT_FALSE(properties.enable_experimental_hw_echo_cancellation);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::goog_audio_mirroring)) {
      EXPECT_FALSE(properties.goog_audio_mirroring);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::goog_auto_gain_control)) {
      EXPECT_TRUE(properties.goog_auto_gain_control);
    }
    if (!Contains(
            exclude_audio_properties,
            &AudioProcessingProperties::goog_experimental_echo_cancellation)) {
      CheckGoogExperimentalEchoCancellationDefault(properties, true);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::goog_typing_noise_detection)) {
      EXPECT_TRUE(properties.goog_typing_noise_detection);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::goog_noise_suppression)) {
      EXPECT_TRUE(properties.goog_noise_suppression);
    }
    if (!Contains(
            exclude_audio_properties,
            &AudioProcessingProperties::goog_experimental_noise_suppression)) {
      EXPECT_TRUE(properties.goog_experimental_noise_suppression);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::goog_beamforming)) {
      EXPECT_TRUE(properties.goog_beamforming);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::goog_highpass_filter)) {
      EXPECT_TRUE(properties.goog_highpass_filter);
    }
    if (!Contains(
            exclude_audio_properties,
            &AudioProcessingProperties::goog_experimental_auto_gain_control)) {
      EXPECT_TRUE(properties.goog_experimental_auto_gain_control);
    }
  }

  void CheckBoolDefaultsContentCapture(
      const AudioSettingsBoolMembers& exclude_main_settings,
      const AudioPropertiesBoolMembers& exclude_audio_properties,
      const AudioCaptureSettings& result) {
    if (!Contains(exclude_main_settings,
                  &AudioCaptureSettings::hotword_enabled)) {
      EXPECT_FALSE(result.hotword_enabled());
    }
    if (!Contains(exclude_main_settings,
                  &AudioCaptureSettings::disable_local_echo)) {
      EXPECT_EQ(GetMediaStreamSource() != kMediaStreamSourceDesktop,
                result.disable_local_echo());
    }
    if (!Contains(exclude_main_settings,
                  &AudioCaptureSettings::render_to_associated_sink)) {
      EXPECT_FALSE(result.render_to_associated_sink());
    }

    const auto& properties = result.audio_processing_properties();
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::enable_sw_echo_cancellation)) {
      EXPECT_FALSE(properties.enable_sw_echo_cancellation);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::disable_hw_echo_cancellation)) {
      EXPECT_TRUE(properties.disable_hw_echo_cancellation);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::
                      enable_experimental_hw_echo_cancellation)) {
      EXPECT_FALSE(properties.enable_experimental_hw_echo_cancellation);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::goog_audio_mirroring)) {
      EXPECT_FALSE(properties.goog_audio_mirroring);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::goog_auto_gain_control)) {
      EXPECT_FALSE(properties.goog_auto_gain_control);
    }
    if (!Contains(
            exclude_audio_properties,
            &AudioProcessingProperties::goog_experimental_echo_cancellation)) {
      EXPECT_FALSE(properties.goog_experimental_echo_cancellation);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::goog_typing_noise_detection)) {
      EXPECT_FALSE(properties.goog_typing_noise_detection);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::goog_noise_suppression)) {
      EXPECT_FALSE(properties.goog_noise_suppression);
    }
    if (!Contains(
            exclude_audio_properties,
            &AudioProcessingProperties::goog_experimental_noise_suppression)) {
      EXPECT_FALSE(properties.goog_experimental_noise_suppression);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::goog_beamforming)) {
      EXPECT_FALSE(properties.goog_beamforming);
    }
    if (!Contains(exclude_audio_properties,
                  &AudioProcessingProperties::goog_highpass_filter)) {
      EXPECT_FALSE(properties.goog_highpass_filter);
    }
    if (!Contains(
            exclude_audio_properties,
            &AudioProcessingProperties::goog_experimental_auto_gain_control)) {
      EXPECT_FALSE(properties.goog_experimental_auto_gain_control);
    }
  }

  void CheckBoolDefaults(
      const AudioSettingsBoolMembers& exclude_main_settings,
      const AudioPropertiesBoolMembers& exclude_audio_properties,
      const AudioCaptureSettings& result) {
    if (IsDeviceCapture()) {
      CheckBoolDefaultsDeviceCapture(exclude_main_settings,
                                     exclude_audio_properties, result);
    } else {
      CheckBoolDefaultsContentCapture(exclude_main_settings,
                                      exclude_audio_properties, result);
    }
  }

  void CheckDevice(const AudioDeviceCaptureCapability& expected_device,
                   const AudioCaptureSettings& result) {
    EXPECT_EQ(expected_device.DeviceID(), result.device_id());
    EXPECT_EQ(expected_device.Parameters().sample_rate(),
              result.device_parameters().sample_rate());
    EXPECT_EQ(expected_device.Parameters().channels(),
              result.device_parameters().channels());
    EXPECT_EQ(expected_device.Parameters().effects(),
              result.device_parameters().effects());
  }

  void CheckDeviceDefaults(const AudioCaptureSettings& result) {
    if (IsDeviceCapture())
      CheckDevice(*default_device_, result);
    else
      EXPECT_TRUE(result.device_id().empty());
  }

  void CheckGeometryDefaults(const AudioCaptureSettings& result) {
    EXPECT_TRUE(
        result.audio_processing_properties().goog_array_geometry.empty());
  }

  void CheckAllDefaults(
      const AudioSettingsBoolMembers& exclude_main_settings,
      const AudioPropertiesBoolMembers& exclude_audio_properties,
      const AudioCaptureSettings& result) {
    CheckBoolDefaults(exclude_main_settings, exclude_audio_properties, result);
    CheckDeviceDefaults(result);
    CheckGeometryDefaults(result);
  }

  // Assumes that echoCancellation is set to true as a basic, exact constraint.
  void CheckAudioProcessingPropertiesForExactEchoCancellationType(
      bool request_hw_echo_cancellation,
      const AudioCaptureSettings& result) {
    const AudioProcessingProperties& properties =
        result.audio_processing_properties();

    // With device capture, the echo_cancellation constraint
    // enables/disables all audio processing by default.
    // With content capture, the echo_cancellation constraint controls
    // only the echo_cancellation properties. The other audio processing
    // properties default to false.
    const bool enable_sw_audio_processing = IsDeviceCapture();

    if (IsDeviceCapture()) {
      EXPECT_NE(request_hw_echo_cancellation,
                properties.enable_sw_echo_cancellation);
      EXPECT_NE(request_hw_echo_cancellation,
                properties.disable_hw_echo_cancellation);
      EXPECT_EQ(request_hw_echo_cancellation,
                properties.enable_experimental_hw_echo_cancellation);
    } else {
      EXPECT_TRUE(properties.enable_sw_echo_cancellation);
      EXPECT_TRUE(properties.disable_hw_echo_cancellation);
      EXPECT_FALSE(properties.enable_experimental_hw_echo_cancellation);
    }
    EXPECT_EQ(enable_sw_audio_processing, properties.goog_auto_gain_control);
    CheckGoogExperimentalEchoCancellationDefault(properties,
                                                 enable_sw_audio_processing);
    EXPECT_EQ(enable_sw_audio_processing,
              properties.goog_typing_noise_detection);
    EXPECT_EQ(enable_sw_audio_processing, properties.goog_noise_suppression);
    EXPECT_EQ(enable_sw_audio_processing,
              properties.goog_experimental_noise_suppression);
    EXPECT_EQ(enable_sw_audio_processing, properties.goog_beamforming);
    EXPECT_EQ(enable_sw_audio_processing, properties.goog_highpass_filter);
    EXPECT_EQ(enable_sw_audio_processing,
              properties.goog_experimental_auto_gain_control);

    // The following are not audio processing.
    EXPECT_FALSE(properties.goog_audio_mirroring);
    EXPECT_FALSE(result.hotword_enabled());
    EXPECT_EQ(GetMediaStreamSource() != kMediaStreamSourceDesktop,
              result.disable_local_echo());
    EXPECT_FALSE(result.render_to_associated_sink());
    if (IsDeviceCapture()) {
      CheckDevice(request_hw_echo_cancellation ? *hw_echo_canceller_device_
                                               : *default_device_,
                  result);
    } else {
      EXPECT_TRUE(result.device_id().empty());
    }
  }

  void CheckAudioProcessingPropertiesForIdealEchoCancellationType(
      const AudioCaptureSettings& result) {
    const AudioProcessingProperties& properties =
        result.audio_processing_properties();

    EXPECT_FALSE(properties.enable_sw_echo_cancellation);
    EXPECT_FALSE(properties.disable_hw_echo_cancellation);
    EXPECT_TRUE(properties.enable_experimental_hw_echo_cancellation);
    EXPECT_TRUE(properties.goog_auto_gain_control);
    CheckGoogExperimentalEchoCancellationDefault(properties, true);
    EXPECT_TRUE(properties.goog_typing_noise_detection);
    EXPECT_TRUE(properties.goog_noise_suppression);
    EXPECT_TRUE(properties.goog_experimental_noise_suppression);
    EXPECT_TRUE(properties.goog_beamforming);
    EXPECT_TRUE(properties.goog_highpass_filter);
    EXPECT_TRUE(properties.goog_experimental_auto_gain_control);

    // The following are not audio processing.
    EXPECT_FALSE(properties.goog_audio_mirroring);
    EXPECT_FALSE(result.hotword_enabled());
    EXPECT_EQ(GetMediaStreamSource() != kMediaStreamSourceDesktop,
              result.disable_local_echo());
    EXPECT_FALSE(result.render_to_associated_sink());
    CheckDevice(*hw_echo_canceller_device_, result);
  }

  MockConstraintFactory constraint_factory_;
  AudioDeviceCaptureCapabilities capabilities_;
  const AudioDeviceCaptureCapability* default_device_ = nullptr;
  const AudioDeviceCaptureCapability* hw_echo_canceller_device_ = nullptr;
  const AudioDeviceCaptureCapability* geometry_device_ = nullptr;
  const std::vector<media::Point> kMicPositions = {{8, 8, 8}, {4, 4, 4}};
  const std::vector<blink::WebString> kEchoCancellationTypeValues = {
      blink::WebString::FromASCII("browser"),
      blink::WebString::FromASCII("system")};

 private:
  // Required for tests involving a MediaStreamAudioSource.
  base::MessageLoop message_loop_;
  MockPeerConnectionDependencyFactory pc_factory_;
};

// The Unconstrained test checks the default selection criteria.
TEST_P(MediaStreamConstraintsUtilAudioTest, Unconstrained) {
  auto result = SelectSettings();

  // All settings should have default values.
  EXPECT_TRUE(result.HasValue());
  CheckAllDefaults(AudioSettingsBoolMembers(), AudioPropertiesBoolMembers(),
                   result);
}

// This test checks all possible ways to set boolean constraints (except
// echo cancellation constraints, which are not mapped 1:1 to output audio
// processing properties).
TEST_P(MediaStreamConstraintsUtilAudioTest, SingleBoolConstraint) {
  // TODO(crbug.com/736309): Use braced initialization instead of push_back once
  // clang has been fixed.
  AudioSettingsBoolMembers kMainSettings;
  kMainSettings.push_back(&AudioCaptureSettings::hotword_enabled);
  kMainSettings.push_back(&AudioCaptureSettings::disable_local_echo);
  kMainSettings.push_back(&AudioCaptureSettings::render_to_associated_sink);

  const std::vector<
      blink::BooleanConstraint blink::WebMediaTrackConstraintSet::*>
      kMainBoolConstraints = {
          &blink::WebMediaTrackConstraintSet::hotword_enabled,
          &blink::WebMediaTrackConstraintSet::disable_local_echo,
          &blink::WebMediaTrackConstraintSet::render_to_associated_sink};

  ASSERT_EQ(kMainSettings.size(), kMainBoolConstraints.size());
  for (auto set_function : kBoolSetFunctions) {
    for (auto accessor : kFactoryAccessors) {
      // Ideal advanced is ignored by the SelectSettings algorithm.
      // Using array elements instead of pointer values due to the comparison
      // failing on some build configurations.
      if (set_function == kBoolSetFunctions[1] &&
          accessor == kFactoryAccessors[1]) {
        continue;
      }
      for (size_t i = 0; i < kMainSettings.size(); ++i) {
        for (bool value : kBoolValues) {
          ResetFactory();
          (((constraint_factory_.*accessor)().*kMainBoolConstraints[i]).*
           set_function)(value);
          auto result = SelectSettings();
          EXPECT_TRUE(result.HasValue());
          EXPECT_EQ(value, (result.*kMainSettings[i])());
          CheckAllDefaults({kMainSettings[i]}, AudioPropertiesBoolMembers(),
                           result);
        }
      }
    }
  }

  const AudioPropertiesBoolMembers kAudioProcessingProperties = {
      &AudioProcessingProperties::goog_audio_mirroring,
      &AudioProcessingProperties::goog_auto_gain_control,
      &AudioProcessingProperties::goog_experimental_echo_cancellation,
      &AudioProcessingProperties::goog_typing_noise_detection,
      &AudioProcessingProperties::goog_noise_suppression,
      &AudioProcessingProperties::goog_experimental_noise_suppression,
      &AudioProcessingProperties::goog_beamforming,
      &AudioProcessingProperties::goog_highpass_filter,
      &AudioProcessingProperties::goog_experimental_auto_gain_control};

  const std::vector<
      blink::BooleanConstraint blink::WebMediaTrackConstraintSet::*>
      kAudioProcessingConstraints = {
          &blink::WebMediaTrackConstraintSet::goog_audio_mirroring,
          &blink::WebMediaTrackConstraintSet::goog_auto_gain_control,
          &blink::WebMediaTrackConstraintSet::
              goog_experimental_echo_cancellation,
          &blink::WebMediaTrackConstraintSet::goog_typing_noise_detection,
          &blink::WebMediaTrackConstraintSet::goog_noise_suppression,
          &blink::WebMediaTrackConstraintSet::
              goog_experimental_noise_suppression,
          &blink::WebMediaTrackConstraintSet::goog_beamforming,
          &blink::WebMediaTrackConstraintSet::goog_highpass_filter,
          &blink::WebMediaTrackConstraintSet::
              goog_experimental_auto_gain_control,
      };

  ASSERT_EQ(kAudioProcessingProperties.size(),
            kAudioProcessingConstraints.size());
  for (auto set_function : kBoolSetFunctions) {
    for (auto accessor : kFactoryAccessors) {
      // Ideal advanced is ignored by the SelectSettings algorithm.
      // Using array elements instead of pointer values due to the comparison
      // failing on some build configurations.
      if (set_function == kBoolSetFunctions[1] &&
          accessor == kFactoryAccessors[1]) {
        continue;
      }
      for (size_t i = 0; i < kAudioProcessingProperties.size(); ++i) {
        for (bool value : kBoolValues) {
          ResetFactory();
          (((constraint_factory_.*accessor)().*kAudioProcessingConstraints[i]).*
           set_function)(value);
          auto result = SelectSettings();
          EXPECT_TRUE(result.HasValue());
          EXPECT_EQ(value, result.audio_processing_properties().*
                               kAudioProcessingProperties[i]);
          CheckAllDefaults(AudioSettingsBoolMembers(),
                           {kAudioProcessingProperties[i]}, result);
        }
      }
    }
  }
}

// DeviceID tests.
TEST_P(MediaStreamConstraintsUtilAudioTest, ExactArbitraryDeviceID) {
  const std::string kArbitraryDeviceID = "arbitrary";
  constraint_factory_.basic().device_id.SetExact(
      blink::WebString::FromASCII(kArbitraryDeviceID));
  auto result = SelectSettings();
  // kArbitraryDeviceID is invalid for device capture, but it is considered
  // valid for content capture. For content capture, validation of device
  // capture is performed by the getUserMedia() implementation.
  if (IsDeviceCapture()) {
    EXPECT_FALSE(result.HasValue());
    EXPECT_EQ(std::string(constraint_factory_.basic().device_id.GetName()),
              std::string(result.failed_constraint_name()));
  } else {
    EXPECT_TRUE(result.HasValue());
    EXPECT_EQ(kArbitraryDeviceID, result.device_id());
    CheckBoolDefaults(AudioSettingsBoolMembers(), AudioPropertiesBoolMembers(),
                      result);
    CheckGeometryDefaults(result);
  }
}

// DeviceID tests check various ways to deal with the device_id constraint.
TEST_P(MediaStreamConstraintsUtilAudioTest, IdealArbitraryDeviceID) {
  const std::string kArbitraryDeviceID = "arbitrary";
  constraint_factory_.basic().device_id.SetIdeal(
      blink::WebString::FromASCII(kArbitraryDeviceID));
  auto result = SelectSettings();
  EXPECT_TRUE(result.HasValue());
  // kArbitraryDeviceID is invalid for device capture, but it is considered
  // valid for content capture. For content capture, validation of device
  // capture is performed by the getUserMedia() implementation.
  if (IsDeviceCapture())
    CheckDeviceDefaults(result);
  else
    EXPECT_EQ(kArbitraryDeviceID, result.device_id());
  CheckBoolDefaults(AudioSettingsBoolMembers(), AudioPropertiesBoolMembers(),
                    result);
  CheckGeometryDefaults(result);
}

TEST_P(MediaStreamConstraintsUtilAudioTest, ExactValidDeviceID) {
  for (const auto& device : capabilities_) {
    constraint_factory_.basic().device_id.SetExact(
        blink::WebString::FromASCII(device.DeviceID()));
    auto result = SelectSettings();
    EXPECT_TRUE(result.HasValue());
    CheckDevice(device, result);
    CheckBoolDefaults(
        AudioSettingsBoolMembers(),
        {&AudioProcessingProperties::enable_sw_echo_cancellation,
         &AudioProcessingProperties::disable_hw_echo_cancellation},
        result);
    bool has_hw_echo_cancellation =
        device.Parameters().effects() & media::AudioParameters::ECHO_CANCELLER;
    EXPECT_EQ(IsDeviceCapture() && !has_hw_echo_cancellation,
              result.audio_processing_properties().enable_sw_echo_cancellation);
    EXPECT_NE(
        IsDeviceCapture() && has_hw_echo_cancellation,
        result.audio_processing_properties().disable_hw_echo_cancellation);
    if (&device == geometry_device_) {
      EXPECT_EQ(kMicPositions,
                result.audio_processing_properties().goog_array_geometry);
    } else {
      CheckGeometryDefaults(result);
    }
  }
}

// Tests the echoCancellation constraint with a device without hardware echo
// cancellation.
TEST_P(MediaStreamConstraintsUtilAudioTest, EchoCancellationWithSw) {
  for (auto set_function : kBoolSetFunctions) {
    for (auto accessor : kFactoryAccessors) {
      // Ideal advanced is ignored by the SelectSettings algorithm.
      // Using array elements instead of pointer values due to the comparison
      // failing on some build configurations.
      if (set_function == kBoolSetFunctions[1] &&
          accessor == kFactoryAccessors[1]) {
        continue;
      }
      for (bool value : kBoolValues) {
        ResetFactory();
        ((constraint_factory_.*accessor)().echo_cancellation.*
         set_function)(value);
        auto result = SelectSettings();
        EXPECT_TRUE(result.HasValue());
        const AudioProcessingProperties& properties =
            result.audio_processing_properties();
        // With device capture, the echo_cancellation constraint
        // enables/disables all audio processing by default.
        // With content capture, the echo_cancellation constraint controls
        // only the echo_cancellation properties. The other audio processing
        // properties default to false.
        bool enable_sw_audio_processing = IsDeviceCapture() ? value : false;
        EXPECT_EQ(value, properties.enable_sw_echo_cancellation);
        EXPECT_TRUE(properties.disable_hw_echo_cancellation);
        EXPECT_EQ(enable_sw_audio_processing,
                  properties.goog_auto_gain_control);
        CheckGoogExperimentalEchoCancellationDefault(
            properties, enable_sw_audio_processing);
        EXPECT_EQ(enable_sw_audio_processing,
                  properties.goog_typing_noise_detection);
        EXPECT_EQ(enable_sw_audio_processing,
                  properties.goog_noise_suppression);
        EXPECT_EQ(enable_sw_audio_processing,
                  properties.goog_experimental_noise_suppression);
        EXPECT_EQ(enable_sw_audio_processing, properties.goog_beamforming);
        EXPECT_EQ(enable_sw_audio_processing, properties.goog_highpass_filter);
        EXPECT_EQ(enable_sw_audio_processing,
                  properties.goog_experimental_auto_gain_control);

        // The following are not audio processing.
        EXPECT_FALSE(properties.goog_audio_mirroring);
        EXPECT_FALSE(result.hotword_enabled());
        EXPECT_EQ(GetMediaStreamSource() != kMediaStreamSourceDesktop,
                  result.disable_local_echo());
        EXPECT_FALSE(result.render_to_associated_sink());
        if (IsDeviceCapture()) {
          CheckDevice(*default_device_, result);
        } else {
          EXPECT_TRUE(result.device_id().empty());
        }
      }
    }
  }
}

// Tests the echoCancellation constraint with a device with hardware echo
// cancellation.
TEST_P(MediaStreamConstraintsUtilAudioTest, EchoCancellationWithHw) {
  // With content capture, there is no hardware echo cancellation, so
  // nothing to test.
  if (!IsDeviceCapture())
    return;

  for (auto set_function : kBoolSetFunctions) {
    for (auto accessor : kFactoryAccessors) {
      // Ideal advanced is ignored by the SelectSettings algorithm.
      // Using array elements instead of pointer values due to the comparison
      // failing on some build configurations.
      if (set_function == kBoolSetFunctions[1] &&
          accessor == kFactoryAccessors[1]) {
        continue;
      }
      for (bool value : kBoolValues) {
        ResetFactory();
        constraint_factory_.basic().device_id.SetExact(
            blink::WebString::FromASCII(hw_echo_canceller_device_->DeviceID()));
        ((constraint_factory_.*accessor)().echo_cancellation.*
         set_function)(value);
        auto result = SelectSettings();
        EXPECT_TRUE(result.HasValue());
        const AudioProcessingProperties& properties =
            result.audio_processing_properties();
        // With hardware echo cancellation, the echo_cancellation constraint
        // enables/disables all audio processing by default, software echo
        // cancellation is always disabled, and hardware echo cancellation is
        // disabled if the echo_cancellation constraint is false.
        EXPECT_FALSE(properties.enable_sw_echo_cancellation);
        EXPECT_EQ(!value, properties.disable_hw_echo_cancellation);
        EXPECT_EQ(value, properties.goog_auto_gain_control);
        CheckGoogExperimentalEchoCancellationDefault(properties, value);
        EXPECT_EQ(value, properties.goog_typing_noise_detection);
        EXPECT_EQ(value, properties.goog_noise_suppression);
        EXPECT_EQ(value, properties.goog_experimental_noise_suppression);
        EXPECT_EQ(value, properties.goog_beamforming);
        EXPECT_EQ(value, properties.goog_highpass_filter);
        EXPECT_EQ(value, properties.goog_experimental_auto_gain_control);

        // The following are not audio processing.
        EXPECT_FALSE(properties.goog_audio_mirroring);
        EXPECT_FALSE(result.hotword_enabled());
        EXPECT_EQ(GetMediaStreamSource() != kMediaStreamSourceDesktop,
                  result.disable_local_echo());
        EXPECT_FALSE(result.render_to_associated_sink());
        CheckDevice(*hw_echo_canceller_device_, result);
      }
    }
  }
}

// Tests the googEchoCancellation constraint with a device without hardware echo
// cancellation.
TEST_P(MediaStreamConstraintsUtilAudioTest, GoogEchoCancellationWithSw) {
  for (auto set_function : kBoolSetFunctions) {
    for (auto accessor : kFactoryAccessors) {
      // Ideal advanced is ignored by the SelectSettings algorithm.
      // Using array elements instead of pointers due to the comparison failing
      // on compilers.
      if (set_function == kBoolSetFunctions[1] &&
          accessor == kFactoryAccessors[1]) {
        continue;
      }
      for (bool value : kBoolValues) {
        ResetFactory();
        ((constraint_factory_.*accessor)().goog_echo_cancellation.*
         set_function)(value);
        auto result = SelectSettings();
        EXPECT_TRUE(result.HasValue());
        const AudioProcessingProperties& properties =
            result.audio_processing_properties();
        // The goog_echo_cancellation constraint controls only the
        // echo_cancellation properties. The other audio processing properties
        // use the default values.
        EXPECT_EQ(value, properties.enable_sw_echo_cancellation);
        EXPECT_TRUE(properties.disable_hw_echo_cancellation);
        CheckBoolDefaults(
            AudioSettingsBoolMembers(),
            {
                &AudioProcessingProperties::enable_sw_echo_cancellation,
                &AudioProcessingProperties::disable_hw_echo_cancellation,
            },
            result);
        if (IsDeviceCapture()) {
          CheckDevice(*default_device_, result);
        } else {
          EXPECT_TRUE(result.device_id().empty());
        }
      }
    }
  }
}

// Tests the googEchoCancellation constraint with a device with hardware echo
// cancellation.
TEST_P(MediaStreamConstraintsUtilAudioTest, GoogEchoCancellationWithHw) {
  // With content capture, there is no hardware echo cancellation, so
  // nothing to test.
  if (!IsDeviceCapture())
    return;

  for (auto set_function : kBoolSetFunctions) {
    for (auto accessor : kFactoryAccessors) {
      // Ideal advanced is ignored by the SelectSettings algorithm.
      // Using array elements instead of pointer values due to the comparison
      // failing on some build configurations.
      if (set_function == kBoolSetFunctions[1] &&
          accessor == kFactoryAccessors[1]) {
        continue;
      }
      for (bool value : kBoolValues) {
        ResetFactory();
        constraint_factory_.basic().device_id.SetExact(
            blink::WebString::FromASCII(hw_echo_canceller_device_->DeviceID()));
        ((constraint_factory_.*accessor)().goog_echo_cancellation.*
         set_function)(value);
        auto result = SelectSettings();
        EXPECT_TRUE(result.HasValue());
        const AudioProcessingProperties& properties =
            result.audio_processing_properties();
        // With hardware echo cancellation, software echo cancellation is always
        // disabled, and hardware echo cancellation is disabled if
        // goog_echo_cancellation is false.
        EXPECT_FALSE(properties.enable_sw_echo_cancellation);
        EXPECT_EQ(!value, properties.disable_hw_echo_cancellation);
        CheckBoolDefaults(
            AudioSettingsBoolMembers(),
            {
                &AudioProcessingProperties::enable_sw_echo_cancellation,
                &AudioProcessingProperties::disable_hw_echo_cancellation,
            },
            result);
        CheckDevice(*hw_echo_canceller_device_, result);
      }
    }
  }
}

// Tests the echoCancellationType constraint without constraining to a device
// with hardware echo cancellation. Tested as basic exact constraints.
TEST_P(MediaStreamConstraintsUtilAudioTest, EchoCancellationTypeExact) {
  for (blink::WebString value : kEchoCancellationTypeValues) {
    const bool request_hw_echo_cancellation =
        value == kEchoCancellationTypeValues[1];

    ResetFactory();
    constraint_factory_.basic().echo_cancellation.SetExact(true);
    constraint_factory_.basic().echo_cancellation_type.SetExact(value);
    auto result = SelectSettings();
    // If content capture and EC type "system", we expect failure.
    if (!IsDeviceCapture() && request_hw_echo_cancellation) {
      EXPECT_FALSE(result.HasValue());
      EXPECT_EQ(result.failed_constraint_name(),
                constraint_factory_.basic().echo_cancellation_type.GetName());
      continue;
    }
    ASSERT_TRUE(result.HasValue());

    CheckAudioProcessingPropertiesForExactEchoCancellationType(
        request_hw_echo_cancellation, result);
  }
}

// Like the test above, but changes the device with hardware echo cancellation
// support to only support experimental hardware echo cancellation. It should
// still be picked if requested.
TEST_P(MediaStreamConstraintsUtilAudioTest,
       EchoCancellationTypeExact_Experimental) {
  if (!IsDeviceCapture())
    return;

  // Replace the device with one that only supports experimental hardware echo
  // cancellation.
  MakeHardwareEchoCancellerDeviceExperimental();

  for (blink::WebString value : kEchoCancellationTypeValues) {
    ResetFactory();
    constraint_factory_.basic().echo_cancellation.SetExact(true);
    constraint_factory_.basic().echo_cancellation_type.SetExact(value);
    auto result = SelectSettings();
    ASSERT_TRUE(result.HasValue());
    const bool request_hw_echo_cancellation =
        value == kEchoCancellationTypeValues[1];
    CheckAudioProcessingPropertiesForExactEchoCancellationType(
        request_hw_echo_cancellation, result);
  }
}

// Tests the echoCancellationType constraint without constraining to a device
// with hardware echo cancellation. Tested as basic ideal constraints.
TEST_P(MediaStreamConstraintsUtilAudioTest, EchoCancellationTypeIdeal) {
  // With content capture, there is no hardware echo cancellation, so
  // nothing to test.
  if (!IsDeviceCapture())
    return;

  constraint_factory_.basic().echo_cancellation.SetExact(true);
  constraint_factory_.basic().echo_cancellation_type.SetIdeal(
      kEchoCancellationTypeValues[1]);
  auto result = SelectSettings();
  ASSERT_TRUE(result.HasValue());
  CheckAudioProcessingPropertiesForIdealEchoCancellationType(result);
}

// Like the test above, but only having a device with experimental hardware echo
// cancellation available.
TEST_P(MediaStreamConstraintsUtilAudioTest,
       EchoCancellationTypeIdeal_Experimental) {
  // With content capture, there is no hardware echo cancellation, so
  // nothing to test.
  if (!IsDeviceCapture())
    return;

  // Replace the device with one that only supports experimental hardware echo
  // cancellation.
  MakeHardwareEchoCancellerDeviceExperimental();

  constraint_factory_.basic().echo_cancellation.SetExact(true);
  constraint_factory_.basic().echo_cancellation_type.SetIdeal(
      kEchoCancellationTypeValues[1]);
  auto result = SelectSettings();
  ASSERT_TRUE(result.HasValue());
  CheckAudioProcessingPropertiesForIdealEchoCancellationType(result);
}

// Tests the echoCancellationType constraint with constraining to a device with
// experimental hardware echo cancellation.
TEST_P(MediaStreamConstraintsUtilAudioTest,
       EchoCancellationTypeWithExpHwDeviceConstraint) {
  // With content capture, there is no system echo cancellation, so
  // nothing to test.
  if (!IsDeviceCapture())
    return;

  MakeHardwareEchoCancellerDeviceExperimental();

  // Include leaving the echoCancellationType constraint unset in the tests.
  // It should then behave as before the constraint was introduced.
  auto echo_cancellation_types_and_unset = kEchoCancellationTypeValues;
  echo_cancellation_types_and_unset.push_back(blink::WebString());

  for (auto set_function : kStringSetFunctions) {
    for (auto accessor : kFactoryAccessors) {
      // Ideal advanced is ignored by the SelectSettings algorithm.
      // Using array elements instead of pointer values due to the comparison
      // failing on some build configurations.
      if (set_function == kStringSetFunctions[1] &&
          accessor == kFactoryAccessors[1]) {
        continue;
      }
      for (blink::WebString ec_type_value : echo_cancellation_types_and_unset) {
        for (bool ec_value : kBoolValues) {
          ResetFactory();
          constraint_factory_.basic().device_id.SetExact(
              blink::WebString::FromASCII(
                  hw_echo_canceller_device_->DeviceID()));
          constraint_factory_.basic().echo_cancellation.SetExact(ec_value);
          if (!ec_type_value.IsNull())
            ((constraint_factory_.*accessor)().echo_cancellation_type.*
             set_function)(ec_type_value);

          // We should get a result if echo cancellation is enabled or if it's
          // disabled and we set the type as an advanced or ideal constraint, or
          // we've left the constraint unset.
          auto result = SelectSettings();
          const bool advanced_constraint = accessor == kFactoryAccessors[1];
          const bool ideal_constraint = set_function == kStringSetFunctions[1];
          const bool should_have_result = ec_value || advanced_constraint ||
                                          ideal_constraint ||
                                          ec_type_value.IsNull();
          EXPECT_EQ(should_have_result, result.HasValue());
          if (!should_have_result)
            continue;

          const AudioProcessingProperties& properties =
              result.audio_processing_properties();

          // With experimental hardware echo cancellation (echo canceller type
          // "system"), the echo_cancellation constraint enables/disables all
          // audio processing by default, software echo cancellation is always
          // disabled, and experimental hardware echo cancellation is disabled
          // if the echo_cancellation constraint is false.
          bool request_hw_echo_cancellation =
              ec_type_value == kEchoCancellationTypeValues[1];

          if (ec_value) {
            EXPECT_NE(request_hw_echo_cancellation,
                      properties.enable_sw_echo_cancellation);
            EXPECT_NE(request_hw_echo_cancellation,
                      properties.disable_hw_echo_cancellation);
            EXPECT_EQ(request_hw_echo_cancellation,
                      properties.enable_experimental_hw_echo_cancellation);
          } else {
            EXPECT_FALSE(properties.enable_sw_echo_cancellation);
            EXPECT_TRUE(properties.disable_hw_echo_cancellation);
            EXPECT_FALSE(properties.enable_experimental_hw_echo_cancellation);
          }
          EXPECT_EQ(ec_value, properties.goog_auto_gain_control);
          CheckGoogExperimentalEchoCancellationDefault(properties, ec_value);
          EXPECT_EQ(ec_value, properties.goog_typing_noise_detection);
          EXPECT_EQ(ec_value, properties.goog_noise_suppression);
          EXPECT_EQ(ec_value, properties.goog_experimental_noise_suppression);
          EXPECT_EQ(ec_value, properties.goog_beamforming);
          EXPECT_EQ(ec_value, properties.goog_highpass_filter);
          EXPECT_EQ(ec_value, properties.goog_experimental_auto_gain_control);

          // The following are not audio processing.
          EXPECT_FALSE(properties.goog_audio_mirroring);
          EXPECT_FALSE(result.hotword_enabled());
          EXPECT_EQ(GetMediaStreamSource() != kMediaStreamSourceDesktop,
                    result.disable_local_echo());
          EXPECT_FALSE(result.render_to_associated_sink());
          CheckDevice(*hw_echo_canceller_device_, result);
        }
      }
    }
  }
}

// Tests the echoCancellationType constraint with constraining to a device
// without experimental hardware echo cancellation, which should fail.
TEST_P(MediaStreamConstraintsUtilAudioTest,
       EchoCancellationTypeWithSwDeviceConstraint) {
  if (!IsDeviceCapture())
    return;

  constraint_factory_.basic().device_id.SetExact(
      blink::WebString::FromASCII(default_device_->DeviceID()));
  constraint_factory_.basic().echo_cancellation.SetExact(true);
  constraint_factory_.basic().echo_cancellation_type.SetExact(
      kEchoCancellationTypeValues[1]);

  auto result = SelectSettings();
  EXPECT_FALSE(result.HasValue());
  EXPECT_EQ(result.failed_constraint_name(),
            constraint_factory_.basic().device_id.GetName());
}

// Test that having differing mandatory values for echoCancellation and
// googEchoCancellation fails.
TEST_P(MediaStreamConstraintsUtilAudioTest, ContradictoryEchoCancellation) {
  for (bool value : kBoolValues) {
    constraint_factory_.basic().echo_cancellation.SetExact(value);
    constraint_factory_.basic().goog_echo_cancellation.SetExact(!value);
    auto result = SelectSettings();
    EXPECT_FALSE(result.HasValue());
    EXPECT_EQ(result.failed_constraint_name(),
              constraint_factory_.basic().echo_cancellation.GetName());
  }
}

// Tests that individual boolean audio-processing constraints override the
// default value set by the echoCancellation constraint.
TEST_P(MediaStreamConstraintsUtilAudioTest,
       EchoCancellationAndSingleBoolConstraint) {
  const AudioPropertiesBoolMembers kAudioProcessingProperties = {
      &AudioProcessingProperties::goog_audio_mirroring,
      &AudioProcessingProperties::goog_auto_gain_control,
      &AudioProcessingProperties::goog_experimental_echo_cancellation,
      &AudioProcessingProperties::goog_typing_noise_detection,
      &AudioProcessingProperties::goog_noise_suppression,
      &AudioProcessingProperties::goog_experimental_noise_suppression,
      &AudioProcessingProperties::goog_beamforming,
      &AudioProcessingProperties::goog_highpass_filter,
      &AudioProcessingProperties::goog_experimental_auto_gain_control};

  const std::vector<
      blink::BooleanConstraint blink::WebMediaTrackConstraintSet::*>
      kAudioProcessingConstraints = {
          &blink::WebMediaTrackConstraintSet::goog_audio_mirroring,
          &blink::WebMediaTrackConstraintSet::goog_auto_gain_control,
          &blink::WebMediaTrackConstraintSet::
              goog_experimental_echo_cancellation,
          &blink::WebMediaTrackConstraintSet::goog_typing_noise_detection,
          &blink::WebMediaTrackConstraintSet::goog_noise_suppression,
          &blink::WebMediaTrackConstraintSet::
              goog_experimental_noise_suppression,
          &blink::WebMediaTrackConstraintSet::goog_beamforming,
          &blink::WebMediaTrackConstraintSet::goog_highpass_filter,
          &blink::WebMediaTrackConstraintSet::
              goog_experimental_auto_gain_control,
      };

  ASSERT_EQ(kAudioProcessingProperties.size(),
            kAudioProcessingConstraints.size());
  for (auto set_function : kBoolSetFunctions) {
    for (auto accessor : kFactoryAccessors) {
      // Ideal advanced is ignored by the SelectSettings algorithm.
      // Using array elements instead of pointer values due to the comparison
      // failing on some build configurations.
      if (set_function == kBoolSetFunctions[1] &&
          accessor == kFactoryAccessors[1]) {
        continue;
      }
      for (size_t i = 0; i < kAudioProcessingProperties.size(); ++i) {
        ResetFactory();
        ((constraint_factory_.*accessor)().echo_cancellation.*
         set_function)(false);
        (((constraint_factory_.*accessor)().*kAudioProcessingConstraints[i]).*
         set_function)(true);
        auto result = SelectSettings();
        EXPECT_TRUE(result.HasValue());
        EXPECT_FALSE(
            result.audio_processing_properties().enable_sw_echo_cancellation);
        EXPECT_TRUE(
            result.audio_processing_properties().disable_hw_echo_cancellation);
        EXPECT_TRUE(result.audio_processing_properties().*
                    kAudioProcessingProperties[i]);
        for (size_t j = 0; j < kAudioProcessingProperties.size(); ++j) {
          if (i == j)
            continue;
          EXPECT_FALSE(result.audio_processing_properties().*
                       kAudioProcessingProperties[j]);
        }
      }
    }
  }
}

// Test advanced constraints sets that can be satisfied.
TEST_P(MediaStreamConstraintsUtilAudioTest, AdvancedCompatibleConstraints) {
  constraint_factory_.AddAdvanced().render_to_associated_sink.SetExact(true);
  constraint_factory_.AddAdvanced().goog_audio_mirroring.SetExact(true);
  auto result = SelectSettings();
  EXPECT_TRUE(result.HasValue());
  CheckDeviceDefaults(result);
  // TODO(crbug.com/736309): Fold this local when clang is fixed.
  auto render_to_associated_sink =
      &AudioCaptureSettings::render_to_associated_sink;
  CheckBoolDefaults({render_to_associated_sink},
                    {&AudioProcessingProperties::goog_audio_mirroring}, result);
  CheckGeometryDefaults(result);
  EXPECT_TRUE(result.render_to_associated_sink());
  EXPECT_TRUE(result.audio_processing_properties().goog_audio_mirroring);
}

// Test that an advanced constraint set that contradicts a previous constraint
// set is ignored, but that further constraint sets that can be satisfied are
// applied.
TEST_P(MediaStreamConstraintsUtilAudioTest,
       AdvancedConflictingMiddleConstraints) {
  constraint_factory_.AddAdvanced().goog_highpass_filter.SetExact(true);
  auto& advanced2 = constraint_factory_.AddAdvanced();
  advanced2.goog_highpass_filter.SetExact(false);
  advanced2.hotword_enabled.SetExact(true);
  constraint_factory_.AddAdvanced().goog_audio_mirroring.SetExact(true);
  auto result = SelectSettings();
  EXPECT_TRUE(result.HasValue());
  CheckDeviceDefaults(result);
  EXPECT_FALSE(result.hotword_enabled());
  // TODO(crbug.com/736309): Fold this local when clang is fixed.
  auto hotword_enabled = &AudioCaptureSettings::hotword_enabled;
  CheckBoolDefaults({hotword_enabled},
                    {&AudioProcessingProperties::goog_audio_mirroring,
                     &AudioProcessingProperties::goog_highpass_filter},
                    result);
  CheckGeometryDefaults(result);
  EXPECT_FALSE(result.hotword_enabled());
  EXPECT_TRUE(result.audio_processing_properties().goog_audio_mirroring);
  EXPECT_TRUE(result.audio_processing_properties().goog_highpass_filter);
}

// Test that an advanced constraint set that contradicts a previous constraint
// set with a boolean constraint is ignored.
TEST_P(MediaStreamConstraintsUtilAudioTest, AdvancedConflictingLastConstraint) {
  constraint_factory_.AddAdvanced().goog_highpass_filter.SetExact(true);
  constraint_factory_.AddAdvanced().hotword_enabled.SetExact(true);
  constraint_factory_.AddAdvanced().goog_audio_mirroring.SetExact(true);
  constraint_factory_.AddAdvanced().hotword_enabled.SetExact(false);
  auto result = SelectSettings();
  EXPECT_TRUE(result.HasValue());
  CheckDeviceDefaults(result);
  // TODO(crbug.com/736309): Fold this local when clang is fixed.
  auto hotword_enabled = &AudioCaptureSettings::hotword_enabled;
  CheckBoolDefaults({hotword_enabled},
                    {&AudioProcessingProperties::goog_audio_mirroring,
                     &AudioProcessingProperties::goog_highpass_filter},
                    result);
  CheckGeometryDefaults(result);
  // The fourth advanced set is ignored because it contradicts the second set.
  EXPECT_TRUE(result.hotword_enabled());
  EXPECT_TRUE(result.audio_processing_properties().goog_audio_mirroring);
  EXPECT_TRUE(result.audio_processing_properties().goog_highpass_filter);
}

// Test that a valid geometry is interpreted correctly in all the ways it can
// be set.
TEST_P(MediaStreamConstraintsUtilAudioTest, ValidGeometry) {
  const blink::WebString kGeometry =
      blink::WebString::FromASCII("-0.02 0 0 0.02 0 1.01");

  for (auto set_function : kStringSetFunctions) {
    for (auto accessor : kFactoryAccessors) {
      // Ideal advanced is ignored by the SelectSettings algorithm.
      // Using array elements instead of pointer values due to the comparison
      // failing on some build configurations.
      if (set_function == kStringSetFunctions[1] &&
          accessor == kFactoryAccessors[1]) {
        continue;
      }
      ResetFactory();
      ((constraint_factory_.*accessor)().goog_array_geometry.*
       set_function)(kGeometry);
      auto result = SelectSettings();
      EXPECT_TRUE(result.HasValue());
      CheckDeviceDefaults(result);
      CheckBoolDefaults(AudioSettingsBoolMembers(),
                        AudioPropertiesBoolMembers(), result);
      const std::vector<media::Point>& geometry =
          result.audio_processing_properties().goog_array_geometry;
      EXPECT_EQ(2u, geometry.size());
      EXPECT_EQ(media::Point(-0.02, 0, 0), geometry[0]);
      EXPECT_EQ(media::Point(0.02, 0, 1.01), geometry[1]);
    }
  }
}

// Test that an invalid geometry is interpreted as empty in all the ways it can
// be set.
TEST_P(MediaStreamConstraintsUtilAudioTest, InvalidGeometry) {
  const blink::WebString kGeometry =
      blink::WebString::FromASCII("1 1 1 invalid");

  for (auto set_function : kStringSetFunctions) {
    for (auto accessor : kFactoryAccessors) {
      // Ideal advanced is ignored by the SelectSettings algorithm.
      // Using array elements instead of pointer values due to the comparison
      // failing on some build configurations.
      if (set_function == kStringSetFunctions[1] &&
          accessor == kFactoryAccessors[1]) {
        continue;
      }
      ResetFactory();
      ((constraint_factory_.*accessor)().goog_array_geometry.*
       set_function)(kGeometry);
      auto result = SelectSettings();
      EXPECT_TRUE(result.HasValue());
      CheckDeviceDefaults(result);
      CheckBoolDefaults(AudioSettingsBoolMembers(),
                        AudioPropertiesBoolMembers(), result);
      const std::vector<media::Point>& geometry =
          result.audio_processing_properties().goog_array_geometry;
      EXPECT_EQ(0u, geometry.size());
    }
  }
}

// Test that an invalid geometry is interpreted as empty in all the ways it can
// be set.
TEST_P(MediaStreamConstraintsUtilAudioTest, DeviceGeometry) {
  if (!IsDeviceCapture())
    return;

  constraint_factory_.basic().device_id.SetExact(
      blink::WebString::FromASCII(geometry_device_->DeviceID()));

  {
    const blink::WebString kValidGeometry =
        blink::WebString::FromASCII("-0.02 0 0  0.02 0 1.01");
    constraint_factory_.basic().goog_array_geometry.SetExact(kValidGeometry);
    auto result = SelectSettings();
    EXPECT_TRUE(result.HasValue());
    CheckDevice(*geometry_device_, result);
    CheckBoolDefaults(AudioSettingsBoolMembers(), AudioPropertiesBoolMembers(),
                      result);
    // Constraints geometry should be preferred over device geometry.
    const std::vector<media::Point>& geometry =
        result.audio_processing_properties().goog_array_geometry;
    EXPECT_EQ(2u, geometry.size());
    EXPECT_EQ(media::Point(-0.02, 0, 0), geometry[0]);
    EXPECT_EQ(media::Point(0.02, 0, 1.01), geometry[1]);
  }

  {
    constraint_factory_.basic().goog_array_geometry.SetExact(
        blink::WebString());
    auto result = SelectSettings();
    EXPECT_TRUE(result.HasValue());
    CheckDevice(*geometry_device_, result);
    CheckBoolDefaults(AudioSettingsBoolMembers(), AudioPropertiesBoolMembers(),
                      result);
    // Empty geometry is valid and should be preferred over device geometry.
    EXPECT_TRUE(
        result.audio_processing_properties().goog_array_geometry.empty());
  }

  {
    const blink::WebString kInvalidGeometry =
        blink::WebString::FromASCII("1 1 1 invalid");
    constraint_factory_.basic().goog_array_geometry.SetExact(kInvalidGeometry);
    auto result = SelectSettings();
    EXPECT_TRUE(result.HasValue());
    CheckDevice(*geometry_device_, result);
    CheckBoolDefaults(AudioSettingsBoolMembers(), AudioPropertiesBoolMembers(),
                      result);
    // Device geometry should be preferred over invalid constraints geometry.
    EXPECT_EQ(kMicPositions,
              result.audio_processing_properties().goog_array_geometry);
  }
}

// NoDevices tests verify that the case with no devices is handled correctly.
TEST_P(MediaStreamConstraintsUtilAudioTest, NoDevicesNoConstraints) {
  // This test makes sense only for device capture.
  if (!IsDeviceCapture())
    return;

  AudioDeviceCaptureCapabilities capabilities;
  auto result = SelectSettingsAudioCapture(
      capabilities, constraint_factory_.CreateWebMediaConstraints(), false);
  EXPECT_FALSE(result.HasValue());
  EXPECT_TRUE(std::string(result.failed_constraint_name()).empty());
}

TEST_P(MediaStreamConstraintsUtilAudioTest, NoDevicesWithConstraints) {
  // This test makes sense only for device capture.
  if (!IsDeviceCapture())
    return;

  AudioDeviceCaptureCapabilities capabilities;
  constraint_factory_.basic().sample_size.SetExact(16);
  auto result = SelectSettingsAudioCapture(
      capabilities, constraint_factory_.CreateWebMediaConstraints(), false);
  EXPECT_FALSE(result.HasValue());
  EXPECT_TRUE(std::string(result.failed_constraint_name()).empty());
}

// Test functionality to support applyConstraints() for tracks attached to
// sources that have no audio processing.
TEST_P(MediaStreamConstraintsUtilAudioTest, SourceWithNoAudioProcessing) {
  for (bool enable_properties : {true, false}) {
    std::unique_ptr<LocalMediaStreamAudioSource> source =
        GetLocalMediaStreamAudioSource(
            enable_properties /* enable_hardware_echo_canceller */,
            enable_properties /* hotword_enabled */,
            enable_properties /* disable_local_echo */,
            enable_properties /* render_to_associated_sink */);

    // These constraints are false in |source|.
    const std::vector<
        blink::BooleanConstraint blink::WebMediaTrackConstraintSet::*>
        kConstraints = {
            &blink::WebMediaTrackConstraintSet::echo_cancellation,
            &blink::WebMediaTrackConstraintSet::hotword_enabled,
            &blink::WebMediaTrackConstraintSet::disable_local_echo,
            &blink::WebMediaTrackConstraintSet::render_to_associated_sink,
        };

    for (size_t i = 0; i < kConstraints.size(); ++i) {
      constraint_factory_.Reset();
      (constraint_factory_.basic().*kConstraints[i])
          .SetExact(enable_properties);
      auto result = SelectSettingsAudioCapture(
          source.get(), constraint_factory_.CreateWebMediaConstraints());
      EXPECT_TRUE(result.HasValue());

      constraint_factory_.Reset();
      (constraint_factory_.basic().*kConstraints[i])
          .SetExact(!enable_properties);
      result = SelectSettingsAudioCapture(
          source.get(), constraint_factory_.CreateWebMediaConstraints());
      EXPECT_FALSE(result.HasValue());

      // Setting just ideal values should always succeed.
      constraint_factory_.Reset();
      (constraint_factory_.basic().*kConstraints[i]).SetIdeal(true);
      result = SelectSettingsAudioCapture(
          source.get(), constraint_factory_.CreateWebMediaConstraints());
      EXPECT_TRUE(result.HasValue());

      constraint_factory_.Reset();
      (constraint_factory_.basic().*kConstraints[i]).SetIdeal(false);
      result = SelectSettingsAudioCapture(
          source.get(), constraint_factory_.CreateWebMediaConstraints());
      EXPECT_TRUE(result.HasValue());
    }
  }
}

// Test functionality to support applyConstraints() for echo cancellation type
// for tracks attached to sources that have no audio processing.
TEST_P(MediaStreamConstraintsUtilAudioTest,
       EchoCancellationTypeWithSourceWithNoAudioProcessing) {
  for (blink::WebString value : kEchoCancellationTypeValues) {
    std::unique_ptr<LocalMediaStreamAudioSource> source =
        GetLocalMediaStreamAudioSource(
            false /* enable_hardware_echo_canceller */,
            false /* hotword_enabled */, false /* disable_local_echo */,
            false /* render_to_associated_sink */);

    // No echo cancellation is available so we expect failure.
    constraint_factory_.Reset();
    constraint_factory_.basic().echo_cancellation_type.SetExact(value);
    auto result = SelectSettingsAudioCapture(
        source.get(), constraint_factory_.CreateWebMediaConstraints());
    EXPECT_FALSE(result.HasValue());

    // Setting just ideal values should always succeed.
    constraint_factory_.Reset();
    constraint_factory_.basic().echo_cancellation_type.SetIdeal(value);
    result = SelectSettingsAudioCapture(
        source.get(), constraint_factory_.CreateWebMediaConstraints());
    EXPECT_TRUE(result.HasValue());
  }
}

// Test functionality to support applyConstraints() for tracks attached to
// sources that have audio processing.
TEST_P(MediaStreamConstraintsUtilAudioTest, SourceWithAudioProcessing) {
  // Processed audio sources are supported only for device capture.
  if (!IsDeviceCapture())
    return;

  for (bool use_defaults : {true, false}) {
    AudioProcessingProperties properties;
    properties.disable_hw_echo_cancellation = true;
    if (!use_defaults) {
      properties.enable_sw_echo_cancellation =
          !properties.enable_sw_echo_cancellation;
      properties.goog_audio_mirroring = !properties.goog_audio_mirroring;
      properties.goog_auto_gain_control = !properties.goog_auto_gain_control;
      properties.goog_experimental_echo_cancellation =
          !properties.goog_experimental_echo_cancellation;
      properties.goog_typing_noise_detection =
          !properties.goog_typing_noise_detection;
      properties.goog_noise_suppression = !properties.goog_noise_suppression;
      properties.goog_experimental_noise_suppression =
          !properties.goog_experimental_noise_suppression;
      properties.goog_beamforming = !properties.goog_beamforming;
      properties.goog_highpass_filter = !properties.goog_highpass_filter;
      properties.goog_experimental_auto_gain_control =
          !properties.goog_experimental_auto_gain_control;
      properties.goog_array_geometry = {{1, 1, 1}, {2, 2, 2}};
    }

    std::unique_ptr<ProcessedLocalAudioSource> source =
        GetProcessedLocalAudioSource(
            properties, use_defaults /* hotword_enabled */,
            use_defaults /* disable_local_echo */,
            use_defaults /* render_to_associated_sink */);
    const std::vector<
        blink::BooleanConstraint blink::WebMediaTrackConstraintSet::*>
        kAudioProcessingConstraints = {
            &blink::WebMediaTrackConstraintSet::echo_cancellation,
            &blink::WebMediaTrackConstraintSet::goog_audio_mirroring,
            &blink::WebMediaTrackConstraintSet::goog_auto_gain_control,
            &blink::WebMediaTrackConstraintSet::
                goog_experimental_echo_cancellation,
            &blink::WebMediaTrackConstraintSet::goog_typing_noise_detection,
            &blink::WebMediaTrackConstraintSet::goog_noise_suppression,
            &blink::WebMediaTrackConstraintSet::
                goog_experimental_noise_suppression,
            &blink::WebMediaTrackConstraintSet::goog_beamforming,
            &blink::WebMediaTrackConstraintSet::goog_highpass_filter,
            &blink::WebMediaTrackConstraintSet::
                goog_experimental_auto_gain_control,
        };
    const AudioPropertiesBoolMembers kAudioProcessingProperties = {
        &AudioProcessingProperties::enable_sw_echo_cancellation,
        &AudioProcessingProperties::goog_audio_mirroring,
        &AudioProcessingProperties::goog_auto_gain_control,
        &AudioProcessingProperties::goog_experimental_echo_cancellation,
        &AudioProcessingProperties::goog_typing_noise_detection,
        &AudioProcessingProperties::goog_noise_suppression,
        &AudioProcessingProperties::goog_experimental_noise_suppression,
        &AudioProcessingProperties::goog_beamforming,
        &AudioProcessingProperties::goog_highpass_filter,
        &AudioProcessingProperties::goog_experimental_auto_gain_control};

    ASSERT_EQ(kAudioProcessingConstraints.size(),
              kAudioProcessingProperties.size());

    for (size_t i = 0; i < kAudioProcessingConstraints.size(); ++i) {
      constraint_factory_.Reset();
      (constraint_factory_.basic().*kAudioProcessingConstraints[i])
          .SetExact(properties.*kAudioProcessingProperties[i]);
      auto result = SelectSettingsAudioCapture(
          source.get(), constraint_factory_.CreateWebMediaConstraints());
      EXPECT_TRUE(result.HasValue());

      constraint_factory_.Reset();
      (constraint_factory_.basic().*kAudioProcessingConstraints[i])
          .SetExact(!(properties.*kAudioProcessingProperties[i]));
      result = SelectSettingsAudioCapture(
          source.get(), constraint_factory_.CreateWebMediaConstraints());
      EXPECT_FALSE(result.HasValue());

      // Setting just ideal values should always succeed.
      constraint_factory_.Reset();
      (constraint_factory_.basic().*kAudioProcessingConstraints[i])
          .SetIdeal(true);
      result = SelectSettingsAudioCapture(
          source.get(), constraint_factory_.CreateWebMediaConstraints());
      EXPECT_TRUE(result.HasValue());

      constraint_factory_.Reset();
      (constraint_factory_.basic().*kAudioProcessingConstraints[i])
          .SetIdeal(false);
      result = SelectSettingsAudioCapture(
          source.get(), constraint_factory_.CreateWebMediaConstraints());
      EXPECT_TRUE(result.HasValue());
    }

    {
      constraint_factory_.Reset();
      // TODO(guidou): Support any semantically equivalent strings for the
      // goog_array_geometry constrainable property. http://crbug.com/796955
      constraint_factory_.basic().goog_array_geometry.SetExact(
          blink::WebString("1 1 1  2 2 2"));
      auto result = SelectSettingsAudioCapture(
          source.get(), constraint_factory_.CreateWebMediaConstraints());
      EXPECT_EQ(result.HasValue(), !use_defaults);

      // Setting ideal should succeed.
      constraint_factory_.Reset();
      constraint_factory_.basic().goog_array_geometry.SetIdeal(
          blink::WebString("4 4 4  5 5 5"));
      result = SelectSettingsAudioCapture(
          source.get(), constraint_factory_.CreateWebMediaConstraints());
      EXPECT_TRUE(result.HasValue());
    }

    // These constraints are false in |source|.
    const std::vector<
        blink::BooleanConstraint blink::WebMediaTrackConstraintSet::*>
        kAudioBrowserConstraints = {
            &blink::WebMediaTrackConstraintSet::hotword_enabled,
            &blink::WebMediaTrackConstraintSet::disable_local_echo,
            &blink::WebMediaTrackConstraintSet::render_to_associated_sink,
        };
    for (size_t i = 0; i < kAudioBrowserConstraints.size(); ++i) {
      constraint_factory_.Reset();
      (constraint_factory_.basic().*kAudioBrowserConstraints[i])
          .SetExact(use_defaults);
      auto result = SelectSettingsAudioCapture(
          source.get(), constraint_factory_.CreateWebMediaConstraints());
      EXPECT_TRUE(result.HasValue());

      constraint_factory_.Reset();
      (constraint_factory_.basic().*kAudioBrowserConstraints[i])
          .SetExact(!use_defaults);
      result = SelectSettingsAudioCapture(
          source.get(), constraint_factory_.CreateWebMediaConstraints());
      EXPECT_FALSE(result.HasValue());

      constraint_factory_.Reset();
      (constraint_factory_.basic().*kAudioBrowserConstraints[i]).SetIdeal(true);
      result = SelectSettingsAudioCapture(
          source.get(), constraint_factory_.CreateWebMediaConstraints());
      EXPECT_TRUE(result.HasValue());

      constraint_factory_.Reset();
      (constraint_factory_.basic().*kAudioBrowserConstraints[i])
          .SetIdeal(false);
      result = SelectSettingsAudioCapture(
          source.get(), constraint_factory_.CreateWebMediaConstraints());
      EXPECT_TRUE(result.HasValue());
    }
  }
}

// Test functionality to support applyConstraints() for echo cancellation type
// for tracks attached to sources that have audio processing.
TEST_P(MediaStreamConstraintsUtilAudioTest,
       EchoCancellationTypeWithSourceWithAudioProcessing) {
  // Processed audio sources are supported only for device capture.
  if (!IsDeviceCapture())
    return;

  for (bool enable_sw_ec : kBoolValues) {
    for (bool enable_exp_hw_ec : kBoolValues) {
      // Both sw and hw echo cancellation can't be enabled.
      if (enable_sw_ec && enable_exp_hw_ec)
        continue;

      AudioProcessingProperties properties;
      properties.DisableDefaultProperties();
      properties.enable_sw_echo_cancellation = enable_sw_ec;
      properties.enable_experimental_hw_echo_cancellation = enable_exp_hw_ec;

      std::unique_ptr<ProcessedLocalAudioSource> source =
          GetProcessedLocalAudioSource(
              properties, false /* hotword_enabled */,
              false /* disable_local_echo */,
              false /* render_to_associated_sink */,
              enable_exp_hw_ec
                  ? media::AudioParameters::PlatformEffectsMask::
                        EXPERIMENTAL_ECHO_CANCELLER
                  : media::AudioParameters::PlatformEffectsMask::NO_EFFECTS);

      for (blink::WebString value : kEchoCancellationTypeValues) {
        bool system_ec_type = value == kEchoCancellationTypeValues[1];

        constraint_factory_.Reset();
        constraint_factory_.basic().echo_cancellation_type.SetExact(value);
        auto result = SelectSettingsAudioCapture(
            source.get(), constraint_factory_.CreateWebMediaConstraints());
        const bool should_have_result_value =
            (enable_sw_ec && !system_ec_type) ||
            (enable_exp_hw_ec && system_ec_type);
        EXPECT_EQ(should_have_result_value, result.HasValue());

        // Setting just ideal values should always succeed.
        constraint_factory_.Reset();
        constraint_factory_.basic().echo_cancellation_type.SetIdeal(value);
        result = SelectSettingsAudioCapture(
            source.get(), constraint_factory_.CreateWebMediaConstraints());
        EXPECT_TRUE(result.HasValue());
      }
    }
  }
}

TEST_P(MediaStreamConstraintsUtilAudioTest, UsedAndUnusedSources) {
  // The distinction of used and unused sources is relevant only for device
  // capture.
  if (!IsDeviceCapture())
    return;

  AudioProcessingProperties properties;
  properties.enable_sw_echo_cancellation = true;

  std::unique_ptr<ProcessedLocalAudioSource> processed_source =
      GetProcessedLocalAudioSource(properties, false /* hotword_enabled */,
                                   false /* disable_local_echo */,
                                   false /* render_to_associated_sink */);

  const std::string kUnusedDeviceID = "unused_device";
  AudioDeviceCaptureCapabilities capabilities;
  capabilities.emplace_back(processed_source.get());
  capabilities.emplace_back(kUnusedDeviceID,
                            media::AudioParameters::UnavailableDeviceParams());

  {
    constraint_factory_.Reset();
    constraint_factory_.basic().echo_cancellation.SetExact(false);

    auto result = SelectSettingsAudioCapture(
        capabilities, constraint_factory_.CreateWebMediaConstraints(),
        false /* should_disable_hardware_noise_suppression */);
    EXPECT_TRUE(result.HasValue());
    EXPECT_EQ(result.device_id(), kUnusedDeviceID);
    EXPECT_FALSE(
        result.audio_processing_properties().enable_sw_echo_cancellation);
  }

  {
    constraint_factory_.Reset();
    constraint_factory_.basic().echo_cancellation.SetExact(true);
    auto result = SelectSettingsAudioCapture(
        capabilities, constraint_factory_.CreateWebMediaConstraints(),
        false /* should_disable_hardware_noise_suppression */);
    EXPECT_TRUE(result.HasValue());
    EXPECT_EQ(result.device_id(), processed_source->device().id);
    EXPECT_TRUE(
        result.audio_processing_properties().enable_sw_echo_cancellation);
  }
}

INSTANTIATE_TEST_CASE_P(,
                        MediaStreamConstraintsUtilAudioTest,
                        testing::Values("",
                                        kMediaStreamSourceTab,
                                        kMediaStreamSourceSystem,
                                        kMediaStreamSourceDesktop));

}  // namespace content
