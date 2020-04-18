// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A simple abstraction for getting the current time during feature extraction.
// This can be mocked out for testing.

#ifndef CHROME_RENDERER_SAFE_BROWSING_FEATURE_EXTRACTOR_CLOCK_H_
#define CHROME_RENDERER_SAFE_BROWSING_FEATURE_EXTRACTOR_CLOCK_H_

#include "base/macros.h"
#include "base/time/time.h"

namespace safe_browsing {

class FeatureExtractorClock {
 public:
  FeatureExtractorClock() {}
  virtual ~FeatureExtractorClock();

  // Returns the current time.  May be mocked for testing.
  virtual base::TimeTicks Now();

 private:
  DISALLOW_COPY_AND_ASSIGN(FeatureExtractorClock);
};

}  // namespace safe_browsing

#endif  // CHROME_RENDERER_SAFE_BROWSING_FEATURE_EXTRACTOR_CLOCK_H_
