// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_HID_DETECTION_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_HID_DETECTION_SCREEN_HANDLER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/values.h"
#include "chrome/browser/chromeos/login/screens/hid_detection_view.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/web_ui.h"

namespace chromeos {

class CoreOobeView;

// WebUI implementation of HIDDetectionScreenView.
class HIDDetectionScreenHandler
    : public HIDDetectionView,
      public BaseScreenHandler {
 public:
  explicit HIDDetectionScreenHandler(CoreOobeView* core_oobe_view);
  ~HIDDetectionScreenHandler() override;

  // HIDDetectionView implementation:
  void Show() override;
  void Hide() override;
  void Bind(HIDDetectionScreen* screen) override;
  void Unbind() override;
  void CheckIsScreenRequired(
      const base::Callback<void(bool)>& on_check_done) override;

  // BaseScreenHandler implementation:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void DeclareJSCallbacks() override;
  void Initialize() override;

  // Registers the preference for derelict state.
  static void RegisterPrefs(PrefRegistrySimple* registry);

 private:
  // JS messages handlers.
  void HandleOnContinue();

  HIDDetectionScreen* screen_ = nullptr;

  CoreOobeView* core_oobe_view_ = nullptr;

  // If true, Initialize() will call Show().
  bool show_on_init_ = false;

  DISALLOW_COPY_AND_ASSIGN(HIDDetectionScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_HID_DETECTION_SCREEN_HANDLER_H_

