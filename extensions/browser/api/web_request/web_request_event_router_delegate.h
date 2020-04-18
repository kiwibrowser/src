// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_EVENT_ROUTER_DELEGATE_H_
#define EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_EVENT_ROUTER_DELEGATE_H_

#include <string>

namespace extensions {

// A delegate class of WebRequestApi that are not a part of chrome.
class WebRequestEventRouterDelegate {
 public:
  virtual ~WebRequestEventRouterDelegate() {}

  // Notifies that a webRequest event that normally would be forwarded to a
  // listener was instead blocked because of withheld permissions.
  virtual void NotifyWebRequestWithheld(int render_process_id,
                                        int render_frame_id,
                                        const std::string& extension_id) = 0;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_EVENT_ROUTER_DELEGATE_H_
