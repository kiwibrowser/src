// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/indicator_spec.h"
#include "chrome/browser/vr/vector_icons/vector_icons.h"
#include "chrome/grit/generated_resources.h"
#include "components/vector_icons/vector_icons.h"

namespace vr {

IndicatorSpec::IndicatorSpec(UiElementName name,
                             UiElementName webvr_name,
                             const gfx::VectorIcon& icon,
                             int resource_string,
                             int background_resource_string,
                             int potential_resource_string,
                             bool CapturingStateModel::*signal,
                             bool CapturingStateModel::*background_signal,
                             bool CapturingStateModel::*potential_signal,
                             bool is_url)
    : name(name),
      webvr_name(webvr_name),
      icon(icon),
      resource_string(resource_string),
      background_resource_string(background_resource_string),
      potential_resource_string(potential_resource_string),
      signal(signal),
      background_signal(background_signal),
      potential_signal(potential_signal),
      is_url(is_url) {}

IndicatorSpec::IndicatorSpec(const IndicatorSpec& other)
    : name(other.name),
      webvr_name(other.webvr_name),
      icon(other.icon),
      resource_string(other.resource_string),
      background_resource_string(other.background_resource_string),
      potential_resource_string(other.potential_resource_string),
      signal(other.signal),
      background_signal(other.background_signal),
      potential_signal(other.potential_signal),
      is_url(other.is_url) {}

IndicatorSpec::~IndicatorSpec() {}

// clang-format off
std::vector<IndicatorSpec> GetIndicatorSpecs() {

  std::vector<IndicatorSpec> specs = {
      {kLocationAccessIndicator, kWebVrLocationAccessIndicator,
       kMyLocationIcon,
       IDS_VR_SHELL_SITE_IS_TRACKING_LOCATION,
       // Background tabs cannot track high accuracy location.
       0,
       IDS_VR_SHELL_SITE_CAN_TRACK_LOCATION,
       &CapturingStateModel::location_access_enabled,
       &CapturingStateModel::background_location_access_enabled,
       &CapturingStateModel::location_access_potentially_enabled,
       false},

      {kAudioCaptureIndicator, kWebVrAudioCaptureIndicator,
       vector_icons::kMicIcon,
       IDS_VR_SHELL_SITE_IS_USING_MICROPHONE,
       IDS_VR_SHELL_BG_IS_USING_MICROPHONE,
       IDS_VR_SHELL_SITE_CAN_USE_MICROPHONE,
       &CapturingStateModel::audio_capture_enabled,
       &CapturingStateModel::background_audio_capture_enabled,
       &CapturingStateModel::audio_capture_potentially_enabled,
       false},

      {kVideoCaptureIndicator, kWebVrVideoCaptureIndicator,
       vector_icons::kVideocamIcon,
       IDS_VR_SHELL_SITE_IS_USING_CAMERA,
       IDS_VR_SHELL_BG_IS_USING_CAMERA,
       IDS_VR_SHELL_SITE_CAN_USE_CAMERA,
       &CapturingStateModel::video_capture_enabled,
       &CapturingStateModel::background_video_capture_enabled,
       &CapturingStateModel::video_capture_potentially_enabled,
       false},

      {kBluetoothConnectedIndicator, kWebVrBluetoothConnectedIndicator,
       vector_icons::kBluetoothConnectedIcon,
       IDS_VR_SHELL_SITE_IS_USING_BLUETOOTH,
       IDS_VR_SHELL_BG_IS_USING_BLUETOOTH,
       IDS_VR_SHELL_SITE_CAN_USE_BLUETOOTH,
       &CapturingStateModel::bluetooth_connected,
       &CapturingStateModel::background_bluetooth_connected,
       &CapturingStateModel::bluetooth_potentially_connected,
       false},

      {kScreenCaptureIndicator, kWebVrScreenCaptureIndicator,
       vector_icons::kScreenShareIcon,
       IDS_VR_SHELL_SITE_IS_SHARING_SCREEN,
       IDS_VR_SHELL_BG_IS_SHARING_SCREEN,
       IDS_VR_SHELL_SITE_CAN_SHARE_SCREEN,
       &CapturingStateModel::screen_capture_enabled,
       &CapturingStateModel::background_screen_capture_enabled,
       &CapturingStateModel::screen_capture_potentially_enabled,
       false}};

  return specs;
}
// clang-format on

}  // namespace vr
