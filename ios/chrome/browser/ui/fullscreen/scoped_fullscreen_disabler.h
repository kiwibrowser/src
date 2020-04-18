// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FULLSCREEN_SCOPED_FULLSCREEN_DISABLER_H_
#define IOS_CHROME_BROWSER_UI_FULLSCREEN_SCOPED_FULLSCREEN_DISABLER_H_

#include "base/logging.h"
#include "base/macros.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller.h"

// A helper object that increments FullscrenController's disabled counter for
// its entire lifetime.
class ScopedFullscreenDisabler {
 public:
  explicit ScopedFullscreenDisabler(FullscreenController* controller)
      : controller_(controller) {
    DCHECK(controller_);
    controller_->IncrementDisabledCounter();
  }
  ~ScopedFullscreenDisabler() { controller_->DecrementDisabledCounter(); }

 private:
  // The FullscreenController being disabled by this object.
  FullscreenController* controller_;

  DISALLOW_COPY_AND_ASSIGN(ScopedFullscreenDisabler);
};

#endif  // IOS_CHROME_BROWSER_UI_FULLSCREEN_SCOPED_FULLSCREEN_DISABLER_H_
