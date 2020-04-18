// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/post_processors/saturated_gain.h"

#include <algorithm>
#include <cmath>

#include "base/logging.h"
#include "base/values.h"
#include "chromecast/base/serializers.h"
#include "chromecast/media/base/slew_volume.h"

AUDIO_POST_PROCESSOR2_SHLIB_CREATE_FUNC(saturated_gain) {
  return new chromecast::media::SaturatedGain(config, num_channels_in);
}

namespace chromecast {
namespace media {

namespace {
const int kNoSampleRate = -1;

// Configuration strings:
const char kGainKey[] = "gain_db";

float DbFsToScale(float db) {
  return std::pow(10, db / 20);
}

}  // namespace

SaturatedGain::SaturatedGain(const std::string& config, int channels)
    : channels_(channels), sample_rate_(kNoSampleRate), last_volume_dbfs_(-1) {
  auto config_dict = base::DictionaryValue::From(DeserializeFromJson(config));
  CHECK(config_dict) << "SaturatedGain config is not valid json: " << config;
  double gain_db;
  CHECK(config_dict->GetDouble(kGainKey, &gain_db)) << config;
  gain_ = DbFsToScale(gain_db);
  LOG(INFO) << "Created a SaturatedGain: gain = " << gain_db << "db";
}

SaturatedGain::~SaturatedGain() = default;

bool SaturatedGain::SetSampleRate(int sample_rate) {
  sample_rate_ = sample_rate;
  slew_volume_.SetSampleRate(sample_rate);
  return true;
}

int SaturatedGain::ProcessFrames(float* data,
                                 int frames,
                                 float volume,
                                 float volume_dbfs) {
  DCHECK(data);

  data_ = data;
  if (volume_dbfs != last_volume_dbfs_) {
    last_volume_dbfs_ = volume_dbfs;
    // Don't apply more gain than attenuation.
    float effective_gain = std::min(DbFsToScale(-last_volume_dbfs_), gain_);
    slew_volume_.SetVolume(effective_gain);
  }

  slew_volume_.ProcessFMUL(false, data, frames, channels_, data);

  return 0;  // No delay in this pipeline.
}

int SaturatedGain::GetRingingTimeInFrames() {
  return 0;
}

int SaturatedGain::NumOutputChannels() {
  return channels_;
}

float* SaturatedGain::GetOutputBuffer() {
  DCHECK(data_);
  return data_;
}

bool SaturatedGain::UpdateParameters(const std::string& message) {
  return false;
}

}  // namespace media
}  // namespace chromecast
