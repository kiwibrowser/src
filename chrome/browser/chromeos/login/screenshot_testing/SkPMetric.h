// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// WARNING! This file is copied from third_party/skia/tools/skpdiff and slightly
// modified to be compilable outside Skia and suit chromium style. Some comments
// can make no sense.
// TODO(elizavetai): remove this file and reuse the original one in Skia

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENSHOT_TESTING_SKPMETRIC_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENSHOT_TESTING_SKPMETRIC_H_

#include "chrome/browser/chromeos/login/screenshot_testing/SkImageDiffer.h"

/**
 * An image differ that uses the pdiff image metric to compare images.
 */

class SkPMetric {
 public:
  virtual bool diff(SkBitmap* baseline,
                    SkBitmap* test,
                    const SkImageDiffer::BitmapsToCreate& bitmapsToCreate,
                    SkImageDiffer::Result* result);

 private:
  typedef SkImageDiffer INHERITED;
};

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENSHOT_TESTING_SKPMETRIC_H_
