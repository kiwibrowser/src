// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_VOICE_INTERACTION_VALUE_PROP_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_VOICE_INTERACTION_VALUE_PROP_SCREEN_HANDLER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/voice_interaction_value_prop_screen_view.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

namespace chromeos {

class VoiceInteractionValuePropScreenHandler
    : public BaseScreenHandler,
      public VoiceInteractionValuePropScreenView {
 public:
  VoiceInteractionValuePropScreenHandler();
  ~VoiceInteractionValuePropScreenHandler() override;

  // BaseScreenHandler:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;

  // VoiceInteractionValuePropScreenView:
  void Bind(VoiceInteractionValuePropScreen* screen) override;
  void Unbind() override;
  void Show() override;
  void Hide() override;

 private:
  // BaseScreenHandler:
  void Initialize() override;

  VoiceInteractionValuePropScreen* screen_ = nullptr;

  // Whether the screen should be shown right after initialization.
  bool show_on_init_ = false;

  DISALLOW_COPY_AND_ASSIGN(VoiceInteractionValuePropScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_VOICE_INTERACTION_VALUE_PROP_SCREEN_HANDLER_H_
