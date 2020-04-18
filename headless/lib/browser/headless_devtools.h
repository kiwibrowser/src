// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_DEVTOOLS_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_DEVTOOLS_H_

#include <memory>

#include "headless/public/headless_browser.h"

namespace headless {

// Starts a DevTools HTTP handler on the loopback interface on the port
// configured by HeadlessBrowser::Options.
void StartLocalDevToolsHttpHandler(HeadlessBrowser::Options* options);
void StopLocalDevToolsHttpHandler();

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_DEVTOOLS_H_
