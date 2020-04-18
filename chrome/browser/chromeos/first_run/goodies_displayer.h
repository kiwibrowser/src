// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FIRST_RUN_GOODIES_DISPLAYER_H_
#define CHROME_BROWSER_CHROMEOS_FIRST_RUN_GOODIES_DISPLAYER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/ui/browser_list_observer.h"

namespace chromeos {
namespace first_run {

struct GoodiesDisplayerTestInfo;

// Handles display of OOBE Goodies page on first display of browser window on
// new Chromebooks.
class GoodiesDisplayer : public BrowserListObserver {
 public:
  // ChromeOS Goodies page for new Chromebook promos.
  static const char kGoodiesURL[];

  // Max days after initial login that we're willing to show Goodies.
  static const int kMaxDaysAfterOobeForGoodies = 14;

  GoodiesDisplayer();
  ~GoodiesDisplayer() override;

  static bool Init();
  static void InitForTesting(GoodiesDisplayerTestInfo* test_info);
  static void Delete();

 private:
  // Overridden from BrowserListObserver.
  void OnBrowserSetLastActive(Browser* browser) override;

  DISALLOW_COPY_AND_ASSIGN(GoodiesDisplayer);
};

// For setup during browser test.
struct GoodiesDisplayerTestInfo {
  GoodiesDisplayerTestInfo();
  ~GoodiesDisplayerTestInfo();

  int days_since_oobe;  // Fake age of device.
  bool setup_complete;  // True when finished, whether GD created or not.
  base::Closure on_setup_complete_callback;  // Called after multithread setup.
};

}  // namespace first_run
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FIRST_RUN_GOODIES_DISPLAYER_H_
