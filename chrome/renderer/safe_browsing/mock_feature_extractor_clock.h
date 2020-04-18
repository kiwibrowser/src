// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A mock implementation of FeatureExtractorClock for testing.

#ifndef CHROME_RENDERER_SAFE_BROWSING_MOCK_FEATURE_EXTRACTOR_CLOCK_H_
#define CHROME_RENDERER_SAFE_BROWSING_MOCK_FEATURE_EXTRACTOR_CLOCK_H_

#include "base/macros.h"
#include "chrome/renderer/safe_browsing/feature_extractor_clock.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace safe_browsing {

class MockFeatureExtractorClock : public FeatureExtractorClock {
 public:
  MockFeatureExtractorClock();
  ~MockFeatureExtractorClock() override;

  MOCK_METHOD0(Now, base::TimeTicks());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockFeatureExtractorClock);
};

}  // namespace safe_browsing

#endif  // CHROME_RENDERER_SAFE_BROWSING_MOCK_FEATURE_EXTRACTOR_CLOCK_H_
