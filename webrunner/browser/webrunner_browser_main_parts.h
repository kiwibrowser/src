// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBRUNNER_BROWSER_WEBRUNNER_BROWSER_MAIN_PARTS_H_
#define WEBRUNNER_BROWSER_WEBRUNNER_BROWSER_MAIN_PARTS_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/browser_main_parts.h"

namespace webrunner {

class WebRunnerBrowserContext;
class WebRunnerScreen;

class WebRunnerBrowserMainParts : public content::BrowserMainParts {
 public:
  WebRunnerBrowserMainParts();
  ~WebRunnerBrowserMainParts() override;

  // content::BrowserMainParts overrides.
  void PreMainMessageLoopRun() override;

 private:
  std::unique_ptr<WebRunnerScreen> screen_;
  std::unique_ptr<WebRunnerBrowserContext> browser_context_;

  DISALLOW_COPY_AND_ASSIGN(WebRunnerBrowserMainParts);
};

}  // namespace webrunner

#endif  // WEBRUNNER_BROWSER_WEBRUNNER_BROWSER_MAIN_PARTS_H_