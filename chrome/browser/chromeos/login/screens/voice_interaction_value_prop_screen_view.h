// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_VOICE_INTERACTION_VALUE_PROP_SCREEN_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_VOICE_INTERACTION_VALUE_PROP_SCREEN_VIEW_H_

#include "chrome/browser/chromeos/login/oobe_screen.h"

namespace chromeos {

class VoiceInteractionValuePropScreen;

// Interface for dependency injection between VoiceInteractionValuePropScreen
// and its WebUI representation.
class VoiceInteractionValuePropScreenView {
 public:
  constexpr static OobeScreen kScreenId =
      OobeScreen::SCREEN_VOICE_INTERACTION_VALUE_PROP;

  virtual ~VoiceInteractionValuePropScreenView() {}

  virtual void Bind(VoiceInteractionValuePropScreen* screen) = 0;
  virtual void Unbind() = 0;
  virtual void Show() = 0;
  virtual void Hide() = 0;

 protected:
  VoiceInteractionValuePropScreenView() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(VoiceInteractionValuePropScreenView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_VOICE_INTERACTION_VALUE_PROP_SCREEN_VIEW_H_
