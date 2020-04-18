// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_WEB_REQUEST_CHROME_EXTENSION_WEB_REQUEST_EVENT_ROUTER_DELEGATE_H_
#define CHROME_BROWSER_EXTENSIONS_API_WEB_REQUEST_CHROME_EXTENSION_WEB_REQUEST_EVENT_ROUTER_DELEGATE_H_

#include <memory>

#include "extensions/browser/api/web_request/web_request_event_router_delegate.h"

class ChromeExtensionWebRequestEventRouterDelegate
    : public extensions::WebRequestEventRouterDelegate {
 public:
  ChromeExtensionWebRequestEventRouterDelegate();
  ~ChromeExtensionWebRequestEventRouterDelegate() override;

  void NotifyWebRequestWithheld(int render_process_id,
                                int render_frame_id,
                                const std::string& extension_id) override;
};

#endif  // CHROME_BROWSER_EXTENSIONS_API_WEB_REQUEST_CHROME_EXTENSION_WEB_REQUEST_EVENT_ROUTER_DELEGATE_H_
