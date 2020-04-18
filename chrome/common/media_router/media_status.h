// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_MEDIA_ROUTER_MEDIA_STATUS_H_
#define CHROME_COMMON_MEDIA_ROUTER_MEDIA_STATUS_H_

#include <string>

#include "base/optional.h"
#include "base/time/time.h"

namespace media_router {

struct HangoutsMediaStatusExtraData {
  // Whether the session associated with the Hangouts MediaRoute is presenting
  // content in "local present" (aka high-bandwidth) mode.
  bool local_present = false;
};

struct MirroringMediaStatusExtraData {
  explicit MirroringMediaStatusExtraData(bool media_remoting_enabled);
  ~MirroringMediaStatusExtraData();

  // Whether media remoting is enabled for mirroring session associated with the
  // MediaRoute.
  bool media_remoting_enabled;
};

// Represents the current state of a media content.
struct MediaStatus {
 public:
  enum class PlayState { PLAYING, PAUSED, BUFFERING };

  MediaStatus();
  MediaStatus(const MediaStatus& other);
  ~MediaStatus();

  MediaStatus& operator=(const MediaStatus& other);
  bool operator==(const MediaStatus& other) const;

  // The main title of the media. For example, in a MediaStatus representing
  // a YouTube Cast session, this could be the title of the video.
  std::string title;

  // If this is true, the media can be played and paused.
  bool can_play_pause = false;

  // If this is true, the media can be muted and unmuted.
  bool can_mute = false;

  // If this is true, the media's volume can be changed.
  bool can_set_volume = false;

  // If this is true, the media's current playback position can be changed.
  bool can_seek = false;

  PlayState play_state = PlayState::PLAYING;

  bool is_muted = false;

  // Current volume of the media, with 1 being the highest and 0 being the
  // lowest/no sound. When |is_muted| is true, there should be no sound
  // regardless of |volume|.
  float volume = 0;

  // The length of the media. A value of zero indicates that this is a media
  // with no set duration (e.g. a live stream).
  base::TimeDelta duration;

  // Current playback position. Must be less than or equal to |duration|.
  base::TimeDelta current_time;

  // Only set for Hangouts routes.
  base::Optional<HangoutsMediaStatusExtraData> hangouts_extra_data;

  // Only set for mirroring routes.
  base::Optional<MirroringMediaStatusExtraData> mirroring_extra_data;
};

}  // namespace media_router

#endif  // CHROME_COMMON_MEDIA_ROUTER_MEDIA_STATUS_H_
