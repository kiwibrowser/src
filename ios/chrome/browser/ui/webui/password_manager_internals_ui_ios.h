// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_WEBUI_PASSWORD_MANAGER_INTERNALS_UI_IOS_H_
#define IOS_CHROME_BROWSER_UI_WEBUI_PASSWORD_MANAGER_INTERNALS_UI_IOS_H_

#include "base/macros.h"
#include "base/values.h"
#include "components/password_manager/core/browser/log_receiver.h"
#include "ios/web/public/web_state/web_state_observer.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"

// The implementation for the chrome://password-manager-internals page.
class PasswordManagerInternalsUIIOS : public web::WebUIIOSController,
                                      public web::WebStateObserver,
                                      public password_manager::LogReceiver {
 public:
  explicit PasswordManagerInternalsUIIOS(web::WebUIIOS* web_ui);
  ~PasswordManagerInternalsUIIOS() override;

  // LogReceiver implementation.
  void LogSavePasswordProgress(const std::string& text) override;

  // web::WebStateObserver implementation.
  void PageLoaded(
      web::WebState* web_state,
      web::PageLoadCompletionStatus load_completion_status) override;
  void WebStateDestroyed(web::WebState* web_state) override;

 private:
  bool registered_with_logging_service_ = false;

  DISALLOW_COPY_AND_ASSIGN(PasswordManagerInternalsUIIOS);
};

#endif  // IOS_CHROME_BROWSER_UI_WEBUI_PASSWORD_MANAGER_INTERNALS_UI_IOS_H_
