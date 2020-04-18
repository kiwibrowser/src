// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webrunner/browser/webrunner_content_browser_client.h"

#include "webrunner/browser/webrunner_browser_main_parts.h"

namespace webrunner {

WebRunnerContentBrowserClient::WebRunnerContentBrowserClient() = default;
WebRunnerContentBrowserClient::~WebRunnerContentBrowserClient() = default;

content::BrowserMainParts*
WebRunnerContentBrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  return new WebRunnerBrowserMainParts();
}

}  // namespace webrunner