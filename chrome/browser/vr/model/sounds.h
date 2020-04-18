// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_SOUNDS_H_
#define CHROME_BROWSER_VR_MODEL_SOUNDS_H_

#include "chrome/browser/vr/model/sound_id.h"

namespace vr {

struct Sounds {
  SoundId hover_enter = kSoundNone;
  SoundId hover_leave = kSoundNone;
  SoundId button_down = kSoundNone;
  SoundId button_up = kSoundNone;
  SoundId move = kSoundNone;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_SOUNDS_H_
