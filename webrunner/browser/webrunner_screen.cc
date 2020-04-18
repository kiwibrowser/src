// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webrunner/browser/webrunner_screen.h"

#include "ui/display/display.h"

namespace webrunner {

WebRunnerScreen::WebRunnerScreen() {
  const int64_t kDefaultDisplayId = 1;
  display::Display display(kDefaultDisplayId);
  ProcessDisplayChanged(display, /*is_primary=*/true);
}

WebRunnerScreen::~WebRunnerScreen() = default;

}  // namespace webrunner
