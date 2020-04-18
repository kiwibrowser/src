// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/chrome/chrome_android_impl.h"

#include <utility>

#include "base/strings/string_split.h"
#include "chrome/test/chromedriver/chrome/device_manager.h"
#include "chrome/test/chromedriver/chrome/devtools_client.h"
#include "chrome/test/chromedriver/chrome/devtools_event_listener.h"
#include "chrome/test/chromedriver/chrome/devtools_http_client.h"
#include "chrome/test/chromedriver/chrome/status.h"

ChromeAndroidImpl::ChromeAndroidImpl(
    std::unique_ptr<DevToolsHttpClient> http_client,
    std::unique_ptr<DevToolsClient> websocket_client,
    std::vector<std::unique_ptr<DevToolsEventListener>>
        devtools_event_listeners,
    std::string page_load_strategy,
    std::unique_ptr<Device> device)
    : ChromeImpl(std::move(http_client),
                 std::move(websocket_client),
                 std::move(devtools_event_listeners),
                 page_load_strategy),
      device_(std::move(device)) {}

ChromeAndroidImpl::~ChromeAndroidImpl() {}

Status ChromeAndroidImpl::GetAsDesktop(ChromeDesktopImpl** desktop) {
  return Status(kUnknownError, "operation is unsupported on Android");
}

std::string ChromeAndroidImpl::GetOperatingSystemName() {
  return "ANDROID";
}

bool ChromeAndroidImpl::HasTouchScreen() const {
  const BrowserInfo* browser_info = GetBrowserInfo();
  if (browser_info->browser_name == "webview")
    return browser_info->major_version >= 44;
  else
    return browser_info->build_no >= 2388;
}

Status ChromeAndroidImpl::QuitImpl() {
  return device_->TearDown();
}

