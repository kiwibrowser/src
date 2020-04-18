// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/safe_browsing/feature_extractor_clock.h"

namespace safe_browsing {

FeatureExtractorClock::~FeatureExtractorClock() {}

base::TimeTicks FeatureExtractorClock::Now() {
  return base::TimeTicks::Now();
}

}  // namespace safe_browsing
