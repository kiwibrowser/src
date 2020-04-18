// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/media_status.h"

namespace media_router {

MirroringMediaStatusExtraData::MirroringMediaStatusExtraData(
    bool media_remoting_enabled)
    : media_remoting_enabled(media_remoting_enabled) {}
MirroringMediaStatusExtraData::~MirroringMediaStatusExtraData() = default;

MediaStatus::MediaStatus() = default;

MediaStatus::MediaStatus(const MediaStatus& other) = default;

MediaStatus::~MediaStatus() = default;

MediaStatus& MediaStatus::operator=(const MediaStatus& other) = default;

bool MediaStatus::operator==(const MediaStatus& other) const {
  return title == other.title &&
         can_play_pause == other.can_play_pause && can_mute == other.can_mute &&
         can_set_volume == other.can_set_volume && can_seek == other.can_seek &&
         play_state == other.play_state && is_muted == other.is_muted &&
         volume == other.volume && duration == other.duration &&
         current_time == other.current_time;
}

}  // namespace media_router
