// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MIRRORING_SERVICE_MIRROR_SETTINGS_H_
#define COMPONENTS_MIRRORING_SERVICE_MIRROR_SETTINGS_H_

#include "base/time/time.h"
#include "base/values.h"
#include "media/capture/video_capture_types.h"
#include "media/cast/cast_config.h"

namespace mirroring {

// Holds the default settings for a mirroring session. This class provides the
// audio/video configs that this sender supports. And also provides the
// audio/video constraints used for capturing.
// TODO(xjz): Add the function to generate the audio capture contraints.
// TODO(xjz): Add setters to the settings that might be overriden by integration
// tests.
class MirrorSettings {
 public:
  MirrorSettings();
  ~MirrorSettings();

  // Get the audio/video config with given codec.
  static media::cast::FrameSenderConfig GetDefaultAudioConfig(
      media::cast::RtpPayloadType payload_type,
      media::cast::Codec codec);
  static media::cast::FrameSenderConfig GetDefaultVideoConfig(
      media::cast::RtpPayloadType payload_type,
      media::cast::Codec codec);

  // Call to override the default resolution settings.
  void SetResolutionContraints(int max_width, int max_height);

  // Get video capture constraints with the current settings.
  media::VideoCaptureParams GetVideoCaptureParams();

  int max_width() const { return max_width_; }
  int max_height() const { return max_height_; }

  // Returns a dictionary value of the current settings.
  base::Value ToDictionaryValue();

 private:
  const int min_width_;
  const int min_height_;
  int max_width_;
  int max_height_;

  DISALLOW_COPY_AND_ASSIGN(MirrorSettings);
};

}  // namespace mirroring

#endif  // COMPONENTS_MIRRORING_SERVICE_MIRROR_SETTINGS_H_
