// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_SOUNDS_MANAGER_AUDIO_DELEGATE_H_
#define CHROME_BROWSER_VR_SOUNDS_MANAGER_AUDIO_DELEGATE_H_

#include <unordered_map>

#include "base/macros.h"
#include "chrome/browser/vr/audio_delegate.h"

namespace vr {

class SoundsManagerAudioDelegate : public AudioDelegate {
 public:
  SoundsManagerAudioDelegate();
  ~SoundsManagerAudioDelegate() override;

  // AudioDelegate implementation.
  void ResetSounds() override;
  bool RegisterSound(SoundId, std::unique_ptr<std::string> data) override;
  void PlaySound(SoundId id) override;

 private:
  std::unordered_map<SoundId, std::unique_ptr<std::string>> sounds_;

  DISALLOW_COPY_AND_ASSIGN(SoundsManagerAudioDelegate);
};

}  //  namespace vr

#endif  // CHROME_BROWSER_VR_SOUNDS_MANAGER_AUDIO_DELEGATE_H_
