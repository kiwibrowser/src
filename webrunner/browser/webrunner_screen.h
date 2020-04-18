// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBRUNNER_BROWSER_WEBRUNNER_SCREEN_H_
#define WEBRUNNER_BROWSER_WEBRUNNER_SCREEN_H_

#include "base/macros.h"

#include "ui/display/screen_base.h"

namespace webrunner {

// display::Screen implementation for WebRunner on Fuchsia.
class DISPLAY_EXPORT WebRunnerScreen : public display::ScreenBase {
 public:
  WebRunnerScreen();
  ~WebRunnerScreen() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebRunnerScreen);
};

}  // namespace webrunner

#endif  // WEBRUNNER_BROWSER_WEBRUNNER_SCREEN_H_
