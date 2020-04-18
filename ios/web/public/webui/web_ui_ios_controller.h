// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_WEBUI_WEB_UI_IOS_CONTROLLER_H_
#define IOS_WEB_PUBLIC_WEBUI_WEB_UI_IOS_CONTROLLER_H_

#include "base/strings/string16.h"

class GURL;

namespace base {
class ListValue;
}

namespace web {

class WebUIIOS;

// A WebUIIOS page is controlled by the embedder's WebUIIOSController object. It
// manages the data source and message handlers.
class WebUIIOSController {
 public:
  explicit WebUIIOSController(WebUIIOS* web_ui) : web_ui_(web_ui) {}
  virtual ~WebUIIOSController() {}

  // Allows the controller to override handling all messages from the page.
  // Return true if the message handling was overridden.
  virtual bool OverrideHandleWebUIIOSMessage(const GURL& source_url,
                                             const std::string& message,
                                             const base::ListValue& args);

  WebUIIOS* web_ui() const { return web_ui_; }

 private:
  WebUIIOS* web_ui_;
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_WEBUI_WEB_UI_IOS_CONTROLLER_H_
