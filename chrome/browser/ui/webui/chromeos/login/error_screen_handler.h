// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ERROR_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ERROR_SCREEN_HANDLER_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/error_screen.h"
#include "chrome/browser/chromeos/login/screens/network_error_view.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/network_dropdown_handler.h"

namespace chromeos {

// A class that handles the WebUI hooks in error screen.
class ErrorScreenHandler : public BaseScreenHandler,
                           public NetworkErrorView,
                           public NetworkDropdownHandler::Observer {
 public:
  ErrorScreenHandler();
  ~ErrorScreenHandler() override;

 private:
  // NetworkErrorView:
  void Show() override;
  void Hide() override;
  void Bind(ErrorScreen* screen) override;
  void Unbind() override;
  void ShowOobeScreen(OobeScreen screen) override;

  // WebUIMessageHandler:
  void RegisterMessages() override;

  // BaseScreenHandler:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void Initialize() override;

  // NetworkDropdownHandler:
  void OnConnectToNetworkRequested() override;

  // WebUI message handlers.
  void HandleHideCaptivePortal();

  // Non-owning ptr.
  ErrorScreen* screen_ = nullptr;

  // Should the screen be shown right after initialization?
  bool show_on_init_ = false;

  // Whether the error screen is currently shown.
  bool showing_ = false;

  base::WeakPtrFactory<ErrorScreenHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ErrorScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ERROR_SCREEN_HANDLER_H_
