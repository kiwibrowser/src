// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_CHROME_NON_BLOCKING_NAVIGATION_TRACKER_H_
#define CHROME_TEST_CHROMEDRIVER_CHROME_NON_BLOCKING_NAVIGATION_TRACKER_H_

#include "chrome/test/chromedriver/chrome/page_load_strategy.h"

class Timeout;
class Status;

class NonBlockingNavigationTracker : public PageLoadStrategy {
public:
  NonBlockingNavigationTracker() {}

  ~NonBlockingNavigationTracker() override;

  // Overriden from PageLoadStrategy:
  Status IsPendingNavigation(const std::string& frame_id,
                             const Timeout* timeout,
                             bool* is_pending) override;
  void set_timed_out(bool timed_out) override;
  bool IsNonBlocking() const override;
};

#endif  // CHROME_TEST_CHROMEDRIVER_CHROME_NON_BLOCKING_NAVIGATION_TRACKER_H_
