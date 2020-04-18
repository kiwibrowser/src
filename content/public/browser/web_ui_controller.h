// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_WEB_UI_CONTROLLER_H_
#define CONTENT_PUBLIC_BROWSER_WEB_UI_CONTROLLER_H_

#include "base/strings/string16.h"
#include "content/common/content_export.h"

class GURL;

namespace base {
class ListValue;
}

namespace content {

class RenderFrameHost;
class WebUI;

// A WebUI page is controller by the embedder's WebUIController object. It
// manages the data source and message handlers.
class CONTENT_EXPORT WebUIController {
 public:
  explicit WebUIController(WebUI* web_ui) : web_ui_(web_ui) {}
  virtual ~WebUIController() {}

  // Allows the controller to override handling all messages from the page.
  // Return true if the message handling was overridden.
  virtual bool OverrideHandleWebUIMessage(const GURL& source_url,
                                          const std::string& message,
                                          const base::ListValue& args);

  // Called when a RenderFrame is created.  This is *not* called for every
  // page load because in some cases a RenderFrame will be reused, for example
  // when reloading or navigating to a same-site URL.
  virtual void RenderFrameCreated(RenderFrameHost* render_frame_host) {}

  WebUI* web_ui() const { return web_ui_; }

 private:
  WebUI* web_ui_;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_WEB_UI_CONTROLLER_H_
