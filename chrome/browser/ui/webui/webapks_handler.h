// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_WEBAPKS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_WEBAPKS_HANDLER_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/android/webapk/webapk_info.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace base {
class ListValue;
}  // namespace base

// Handles JavaScript messages from the chrome://webapks page.
class WebApksHandler : public content::WebUIMessageHandler {
 public:
  WebApksHandler();
  ~WebApksHandler() override;

  // content::WebUIMessageHandler:
  void RegisterMessages() override;

  // Handler for the "requestWebApksInfo" message. This requests
  // information for the installed WebAPKs and returns it to JS using
  // OnWebApkInfoReceived().
  virtual void HandleRequestWebApksInfo(const base::ListValue* args);

 private:
  // Sends information for the installed WebAPKs to JS.
  void OnWebApkInfoRetrieved(const std::vector<WebApkInfo>& webapks_list);

  // Factory for the creating refs in callbacks.
  base::WeakPtrFactory<WebApksHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebApksHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_WEBAPKS_HANDLER_H_
