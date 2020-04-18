// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SIGNIN_INLINE_LOGIN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SIGNIN_INLINE_LOGIN_HANDLER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace base {
class DictionaryValue;
}

namespace signin_metrics {
enum class AccessPoint;
}

// The base class handler for the inline login WebUI.
class InlineLoginHandler : public content::WebUIMessageHandler {
 public:
  InlineLoginHandler();
  ~InlineLoginHandler() override;

  // content::WebUIMessageHandler overrides:
  void RegisterMessages() override;

 protected:
  // Enum for gaia auth mode, must match AuthMode defined in
  // chrome/browser/resources/gaia_auth_host/gaia_auth_host.js.
  enum AuthMode {
    kDefaultAuthMode = 0,
    kOfflineAuthMode = 1,
    kDesktopAuthMode = 2
  };

  // Closes the dialog by calling the |inline.login.closeDialog| Javascript
  // function.
  // Does nothing if calling Javascript functions is not allowed.
  void CloseDialogFromJavascript();

 private:
  // JS callback to prepare for starting auth.
  void HandleInitializeMessage(const base::ListValue* args);

  // Continue to initialize the gaia auth extension. It calls
  // |SetExtraInitParams| to set extra init params.
  void ContinueHandleInitializeMessage();

  // JS callback to complete login. It calls |CompleteLogin| to do the real
  // work.
  void HandleCompleteLoginMessage(const base::ListValue* args);

  // JS callback to switch the UI from a constrainted dialog to a full tab.
  void HandleSwitchToFullTabMessage(const base::ListValue* args);

  // Handles the web ui message sent when the navigation button is clicked by
  // the user, requesting either a back navigation or closing the dialog.
  void HandleNavigationButtonClicked(const base::ListValue* args);

  // Handles the web ui message sent when the window is closed from javascript.
  void HandleDialogClose(const base::ListValue* args);

  virtual void SetExtraInitParams(base::DictionaryValue& params) {}
  virtual void CompleteLogin(const base::ListValue* args) = 0;

  base::WeakPtrFactory<InlineLoginHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(InlineLoginHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SIGNIN_INLINE_LOGIN_HANDLER_H_
