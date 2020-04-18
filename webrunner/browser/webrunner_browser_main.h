// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBRUNNER_BROWSER_WEBRUNNER_BROWSER_MAIN_H_
#define WEBRUNNER_BROWSER_WEBRUNNER_BROWSER_MAIN_H_

#include <memory>

namespace content {
struct MainFunctionParams;
}  // namespace content

namespace webrunner {
int WebRunnerBrowserMain(const content::MainFunctionParams& parameters);
}  // namespace webrunner

#endif  // WEBRUNNER_BROWSER_WEBRUNNER_BROWSER_MAIN_H_
