// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/mojo/media_status_struct_traits.h"

#include "base/strings/string_util.h"
#include "mojo/public/cpp/base/time_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<media_router::mojom::MediaStatusDataView,
                  media_router::MediaStatus>::
    Read(media_router::mojom::MediaStatusDataView data,
         media_router::MediaStatus* out) {
  if (!data.ReadTitle(&out->title) || !base::IsStringUTF8(out->title))
    return false;

  out->can_play_pause = data.can_play_pause();
  out->can_mute = data.can_mute();
  out->can_set_volume = data.can_set_volume();
  out->can_seek = data.can_seek();

  if (!data.ReadPlayState(&out->play_state))
    return false;

  out->is_muted = data.is_muted();
  out->volume = data.volume();

  if (!data.ReadDuration(&out->duration))
    return false;

  if (!data.ReadCurrentTime(&out->current_time))
    return false;

  if (!data.ReadHangoutsExtraData(&out->hangouts_extra_data))
    return false;

  return true;
}

bool StructTraits<media_router::mojom::HangoutsMediaStatusExtraDataDataView,
                  media_router::HangoutsMediaStatusExtraData>::
    Read(media_router::mojom::HangoutsMediaStatusExtraDataDataView data,
         media_router::HangoutsMediaStatusExtraData* out) {
  out->local_present = data.local_present();
  return true;
}

}  // namespace mojo
