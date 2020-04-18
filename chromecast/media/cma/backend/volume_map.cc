// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/volume_map.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/values.h"
#include "chromecast/base/serializers.h"
#include "chromecast/media/cma/backend/cast_audio_json.h"

namespace chromecast {
namespace media {

namespace {
constexpr char kKeyVolumeMap[] = "volume_map";
constexpr char kKeyLevel[] = "level";
constexpr char kKeyDb[] = "db";
constexpr float kMinDbFS = -120.0f;

}  // namespace

VolumeMap::VolumeMap() {
  auto cast_audio_config =
      DeserializeJsonFromFile(base::FilePath(kCastAudioJsonFilePath));
  const base::DictionaryValue* cast_audio_dict;
  if (!cast_audio_config ||
      !cast_audio_config->GetAsDictionary(&cast_audio_dict)) {
    LOG(WARNING) << "No cast audio config found; using default volume map.";
    UseDefaultVolumeMap();
    return;
  }

  const base::ListValue* volume_map_list;
  if (!cast_audio_dict->GetList(kKeyVolumeMap, &volume_map_list)) {
    LOG(WARNING) << "No volume map found; using default volume map.";
    UseDefaultVolumeMap();
    return;
  }

  double prev_level = -1.0;
  for (size_t i = 0; i < volume_map_list->GetSize(); ++i) {
    const base::DictionaryValue* volume_map_entry;
    CHECK(volume_map_list->GetDictionary(i, &volume_map_entry));

    double level;
    CHECK(volume_map_entry->GetDouble(kKeyLevel, &level));
    CHECK_GE(level, 0.0);
    CHECK_LE(level, 1.0);
    CHECK_GT(level, prev_level);
    prev_level = level;

    double db;
    CHECK(volume_map_entry->GetDouble(kKeyDb, &db));
    CHECK_LE(db, 0.0);
    if (level == 1.0) {
      CHECK_EQ(db, 0.0);
    }

    volume_map_.push_back({level, db});
  }

  if (volume_map_.empty()) {
    LOG(FATAL) << "No entries in volume map.";
    return;
  }

  if (volume_map_[0].level > 0.0) {
    volume_map_.insert(volume_map_.begin(), {0.0, kMinDbFS});
  }

  if (volume_map_.rbegin()->level < 1.0) {
    volume_map_.push_back({1.0, 0.0});
  }
}

VolumeMap::~VolumeMap() = default;

float VolumeMap::VolumeToDbFS(float volume) {
  if (volume <= volume_map_[0].level) {
    return volume_map_[0].db;
  }
  for (size_t i = 1; i < volume_map_.size(); ++i) {
    if (volume < volume_map_[i].level) {
      const float x_range = volume_map_[i].level - volume_map_[i - 1].level;
      const float y_range = volume_map_[i].db - volume_map_[i - 1].db;
      const float x_pos = volume - volume_map_[i - 1].level;

      return volume_map_[i - 1].db + x_pos * y_range / x_range;
    }
  }
  return volume_map_[volume_map_.size() - 1].db;
}

float VolumeMap::DbFSToVolume(float db) {
  if (db <= volume_map_[0].db) {
    return volume_map_[0].level;
  }
  for (size_t i = 1; i < volume_map_.size(); ++i) {
    if (db < volume_map_[i].db) {
      const float x_range = volume_map_[i].db - volume_map_[i - 1].db;
      const float y_range = volume_map_[i].level - volume_map_[i - 1].level;
      const float x_pos = db - volume_map_[i - 1].db;

      return volume_map_[i - 1].level + x_pos * y_range / x_range;
    }
  }
  return volume_map_[volume_map_.size() - 1].level;
}

// static
void VolumeMap::UseDefaultVolumeMap() {
  volume_map_ = {{0.0f, kMinDbFS},
                 {0.01f, -58.0f},
                 {0.090909f, -48.0f},
                 {0.818182f, -8.0f},
                 {1.0f, 0.0f}};
}

}  // namespace media
}  // namespace chromecast
