// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "chromecast/public/cast_media_shlib.h"
#include "chromecast/public/media/decoder_config.h"
#include "chromecast/public/media/media_capabilities_shlib.h"

namespace chromecast {
namespace media {

void CastMediaShlib::Initialize(const std::vector<std::string>& argv) {}

void CastMediaShlib::Finalize() {}

VideoPlane* CastMediaShlib::GetVideoPlane() {
  return nullptr;
}

MediaPipelineBackend* CastMediaShlib::CreateMediaPipelineBackend(
    const MediaPipelineDeviceParams& params) {
  return nullptr;
}

double CastMediaShlib::GetMediaClockRate() {
  return 0.0;
}

double CastMediaShlib::MediaClockRatePrecision() {
  return 0.0;
}

void CastMediaShlib::MediaClockRateRange(double* minimum_rate,
                                         double* maximum_rate) {}

bool CastMediaShlib::SetMediaClockRate(double new_rate) {
  return false;
}

bool CastMediaShlib::SupportsMediaClockRateChange() {
  return false;
}

bool MediaCapabilitiesShlib::IsSupportedVideoConfig(VideoCodec codec,
                                                    VideoProfile profile,
                                                    int level) {
  // This should not be called directly.
  NOTREACHED() << "Unexpected call to "
               << "MediaCapabilitiesShlib::IsSupportedVideoConfig on Android";
  return false;
}

bool MediaCapabilitiesShlib::IsSupportedAudioConfig(const AudioConfig& config) {
  // This should not be called directly.
  NOTREACHED() << "Unexpected call to "
               << "MediaCapabilitiesShlib::IsSupportedAudioConfig on Android";
  return false;
}

}  // namespace media
}  // namespace chromecast
