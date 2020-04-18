// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webrunner/browser/webrunner_browser_main_parts.h"

#include "webrunner/browser/webrunner_browser_context.h"
#include "webrunner/browser/webrunner_screen.h"

namespace webrunner {

WebRunnerBrowserMainParts::WebRunnerBrowserMainParts() = default;
WebRunnerBrowserMainParts::~WebRunnerBrowserMainParts() = default;

void WebRunnerBrowserMainParts::PreMainMessageLoopRun() {
  DCHECK(!screen_);
  screen_ = std::make_unique<WebRunnerScreen>();
  display::Screen::SetScreenInstance(screen_.get());

  DCHECK(!browser_context_);
  browser_context_ = std::make_unique<WebRunnerBrowserContext>();
}

}  // namespace webrunner
