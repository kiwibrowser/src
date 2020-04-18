// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_UPDATE_REQUIRED_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_UPDATE_REQUIRED_SCREEN_HANDLER_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/update_required_screen.h"
#include "chrome/browser/chromeos/login/screens/update_required_view.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

namespace chromeos {

class UpdateRequiredScreenHandler : public UpdateRequiredView,
                                    public BaseScreenHandler {
 public:
  UpdateRequiredScreenHandler();
  ~UpdateRequiredScreenHandler() override;

 private:
  void Show() override;
  void Hide() override;
  void Bind(UpdateRequiredScreen* screen) override;
  void Unbind() override;

  // BaseScreenHandler:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void Initialize() override;

  UpdateRequiredScreen* screen_ = nullptr;

  // If true, Initialize() will call Show().
  bool show_on_init_ = false;

  DISALLOW_COPY_AND_ASSIGN(UpdateRequiredScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_UPDATE_REQUIRED_SCREEN_HANDLER_H_
