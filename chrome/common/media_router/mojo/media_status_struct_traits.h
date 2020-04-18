// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_MEDIA_ROUTER_MOJO_MEDIA_STATUS_STRUCT_TRAITS_H_
#define CHROME_COMMON_MEDIA_ROUTER_MOJO_MEDIA_STATUS_STRUCT_TRAITS_H_

#include <string>

#include "chrome/common/media_router/media_status.h"
#include "chrome/common/media_router/mojo/media_status.mojom.h"

namespace mojo {

template <>
struct EnumTraits<media_router::mojom::MediaStatus::PlayState,
                  media_router::MediaStatus::PlayState> {
  static media_router::mojom::MediaStatus::PlayState ToMojom(
      media_router::MediaStatus::PlayState play_state) {
    switch (play_state) {
      case media_router::MediaStatus::PlayState::PLAYING:
        return media_router::mojom::MediaStatus::PlayState::PLAYING;
      case media_router::MediaStatus::PlayState::PAUSED:
        return media_router::mojom::MediaStatus::PlayState::PAUSED;
      case media_router::MediaStatus::PlayState::BUFFERING:
        return media_router::mojom::MediaStatus::PlayState::BUFFERING;
    }
    NOTREACHED() << "Unknown play state " << static_cast<int>(play_state);
    return media_router::mojom::MediaStatus::PlayState::PLAYING;
  }

  static bool FromMojom(media_router::mojom::MediaStatus::PlayState input,
                        media_router::MediaStatus::PlayState* output) {
    switch (input) {
      case media_router::mojom::MediaStatus::PlayState::PLAYING:
        *output = media_router::MediaStatus::PlayState::PLAYING;
        return true;
      case media_router::mojom::MediaStatus::PlayState::PAUSED:
        *output = media_router::MediaStatus::PlayState::PAUSED;
        return true;
      case media_router::mojom::MediaStatus::PlayState::BUFFERING:
        *output = media_router::MediaStatus::PlayState::BUFFERING;
        return true;
    }
    NOTREACHED() << "Unknown play state " << static_cast<int>(input);
    return false;
  }
};

template <>
struct StructTraits<media_router::mojom::MediaStatusDataView,
                    media_router::MediaStatus> {
  static bool Read(media_router::mojom::MediaStatusDataView data,
                   media_router::MediaStatus* out);

  static const std::string& title(const media_router::MediaStatus& status) {
    return status.title;
  }

  static bool can_play_pause(const media_router::MediaStatus& status) {
    return status.can_play_pause;
  }

  static bool can_mute(const media_router::MediaStatus& status) {
    return status.can_mute;
  }

  static bool can_set_volume(const media_router::MediaStatus& status) {
    return status.can_set_volume;
  }

  static bool can_seek(const media_router::MediaStatus& status) {
    return status.can_seek;
  }

  static media_router::MediaStatus::PlayState play_state(
      const media_router::MediaStatus& status) {
    return status.play_state;
  }

  static bool is_muted(const media_router::MediaStatus& status) {
    return status.is_muted;
  }

  static float volume(const media_router::MediaStatus& status) {
    return status.volume;
  }

  static base::TimeDelta duration(const media_router::MediaStatus& status) {
    return status.duration;
  }

  static base::TimeDelta current_time(const media_router::MediaStatus& status) {
    return status.current_time;
  }

  static const base::Optional<media_router::HangoutsMediaStatusExtraData>&
  hangouts_extra_data(const media_router::MediaStatus& status) {
    return status.hangouts_extra_data;
  }
};

template <>
struct StructTraits<media_router::mojom::HangoutsMediaStatusExtraDataDataView,
                    media_router::HangoutsMediaStatusExtraData> {
  static bool Read(
      media_router::mojom::HangoutsMediaStatusExtraDataDataView data,
      media_router::HangoutsMediaStatusExtraData* out);

  static bool local_present(
      const media_router::HangoutsMediaStatusExtraData& data) {
    return data.local_present;
  }
};

}  // namespace mojo

#endif  // CHROME_COMMON_MEDIA_ROUTER_MOJO_MEDIA_STATUS_STRUCT_TRAITS_H_
