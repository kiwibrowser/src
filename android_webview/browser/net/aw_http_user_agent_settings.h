// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_NET_AW_HTTP_USER_AGENT_SETTINGS_H_
#define ANDROID_WEBVIEW_BROWSER_NET_AW_HTTP_USER_AGENT_SETTINGS_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "net/url_request/http_user_agent_settings.h"

namespace android_webview {

// An implementation of |HttpUserAgentSettings| that provides HTTP header
// Accept-Language value that tracks aw accept language.
class AwHttpUserAgentSettings : public net::HttpUserAgentSettings {
 public:
  // Must be called on the UI thread.
  AwHttpUserAgentSettings();
  // Must be called on the IO thread.
  ~AwHttpUserAgentSettings() override;

  // net::HttpUserAgentSettings implementation
  std::string GetAcceptLanguage() const override;
  std::string GetUserAgent() const override;

 private:
  // Avoid re-processing by caching the last value from the locale and the
  // last result of processing via net::HttpUtil::GenerateAccept*Header().
  mutable std::string last_aw_accept_language_;
  mutable std::string last_http_accept_language_;

  DISALLOW_COPY_AND_ASSIGN(AwHttpUserAgentSettings);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_NET_AW_HTTP_USER_AGENT_SETTINGS_H_

