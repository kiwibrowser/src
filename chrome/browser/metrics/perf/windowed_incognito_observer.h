// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_PERF_WINDOWED_INCOGNITO_OBSERVER_H_
#define CHROME_BROWSER_METRICS_PERF_WINDOWED_INCOGNITO_OBSERVER_H_

#include "base/macros.h"
#include "chrome/browser/ui/browser_list_observer.h"

class Browser;

namespace metrics {

// This class watches for any incognito window being opened from the time it is
// instantiated to the time it is destroyed.
class WindowedIncognitoObserver : public BrowserListObserver {
 public:
  WindowedIncognitoObserver();
  ~WindowedIncognitoObserver() override;

  // This method can be checked to see whether any incognito window has been
  // opened since the time this object was created.
  bool incognito_launched() const {
    return incognito_launched_;
  }

 protected:
  // For testing.
  void set_incognito_launched(bool value) {
    incognito_launched_ = value;
  }

 private:
  // BrowserListObserver implementation.
  void OnBrowserAdded(Browser* browser) override;

  // Gets set if an incognito window was opened during the lifetime of the
  // object. Closing the window does not clear the flag.
  bool incognito_launched_;

  DISALLOW_COPY_AND_ASSIGN(WindowedIncognitoObserver);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_PERF_WINDOWED_INCOGNITO_OBSERVER_H_
