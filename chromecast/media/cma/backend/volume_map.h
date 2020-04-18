// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_VOLUME_MAP_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_VOLUME_MAP_H_

#include <vector>

#include "base/macros.h"

namespace chromecast {
namespace media {

class VolumeMap {
 public:
  VolumeMap();
  ~VolumeMap();

  float VolumeToDbFS(float volume);

  float DbFSToVolume(float db);

 private:
  struct LevelToDb {
    float level;
    float db;
  };

  void UseDefaultVolumeMap();

  std::vector<LevelToDb> volume_map_;

  DISALLOW_COPY_AND_ASSIGN(VolumeMap);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_VOLUME_MAP_H_
