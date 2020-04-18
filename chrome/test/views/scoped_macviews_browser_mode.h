// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_VIEWS_SCOPED_MACVIEWS_BROWSER_MODE_H_
#define CHROME_TEST_VIEWS_SCOPED_MACVIEWS_BROWSER_MODE_H_

#include "base/macros.h"
#include "base/test/scoped_feature_list.h"

namespace test {

// TODO(ellyjones): Delete this class once Mac Chrome always uses a Views
// browser window.
//
// This is a transitional class, designed for forcing a test case to run with a
// specific type of browser window on the Mac. This class is available on all
// platforms so that every use of it doesn't have to be wrapped in preprocessor
// conditionals, but is a no-op on non-Mac platforms.
//
// Tests that care about using a specific type of browser window, or that only
// work with one type of browser window, can include a member variable of type
// ScopedMacViewsBrowserMode to enforce that.
class ScopedMacViewsBrowserMode {
 public:
  explicit ScopedMacViewsBrowserMode(bool is_views);
  virtual ~ScopedMacViewsBrowserMode();

 private:
  base::test::ScopedFeatureList feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ScopedMacViewsBrowserMode);
};

}  // namespace test

#endif  // CHROME_TEST_VIEWS_SCOPED_MACVIEWS_BROWSER_MODE_H_
