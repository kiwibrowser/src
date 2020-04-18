// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_VOICE_INTERACTION_VALUE_PROP_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_VOICE_INTERACTION_VALUE_PROP_SCREEN_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"

namespace arc {
class ArcVoiceInteractionArcHomeService;
}

namespace chromeos {

class VoiceInteractionValuePropScreenView;
class BaseScreenDelegate;

class VoiceInteractionValuePropScreen : public BaseScreen {
 public:
  VoiceInteractionValuePropScreen(BaseScreenDelegate* base_screen_delegate,
                                  VoiceInteractionValuePropScreenView* view);
  ~VoiceInteractionValuePropScreen() override;

  // Called when view is destroyed so there's no dead reference to it.
  void OnViewDestroyed(VoiceInteractionValuePropScreenView* view_);

  // BaseScreen:
  void Show() override;
  void Hide() override;
  void OnUserAction(const std::string& action_id) override;

 private:
  void OnSkipPressed();
  void OnNextPressed();

  arc::ArcVoiceInteractionArcHomeService* GetVoiceInteractionHomeService();

  VoiceInteractionValuePropScreenView* view_;

  DISALLOW_COPY_AND_ASSIGN(VoiceInteractionValuePropScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_VOICE_INTERACTION_VALUE_PROP_SCREEN_H_
