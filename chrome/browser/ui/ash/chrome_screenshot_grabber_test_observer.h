// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_CHROME_SCREENSHOT_GRABBER_TEST_OBSERVER_H_
#define CHROME_BROWSER_UI_ASH_CHROME_SCREENSHOT_GRABBER_TEST_OBSERVER_H_

#include "ui/snapshot/screenshot_grabber.h"

// Testing interface to ChromeScreenshotGrabber.
class ChromeScreenshotGrabberTestObserver {
 public:
  // Dispatched after attempting to take a screenshot with the |result| and
  // |screenshot_path| of the taken screenshot (if successful).
  virtual void OnScreenshotCompleted(ui::ScreenshotResult screenshot_result,
                                     const base::FilePath& screenshot_path) = 0;

 protected:
  virtual ~ChromeScreenshotGrabberTestObserver() {}
};

#endif  // CHROME_BROWSER_UI_ASH_CHROME_SCREENSHOT_GRABBER_TEST_OBSERVER_H_
