// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROME_BROWSER_FIELD_TRIALS_H_
#define CHROME_BROWSER_CHROME_BROWSER_FIELD_TRIALS_H_

#include "base/macros.h"
#include "components/variations/platform_field_trials.h"

namespace base {
class FeatureList;
}

class ChromeBrowserFieldTrials : public variations::PlatformFieldTrials {
 public:
  ChromeBrowserFieldTrials();
  ~ChromeBrowserFieldTrials() override;

  // variations::PlatformFieldTrials:
  void SetupFieldTrials() override;
  void SetupFeatureControllingFieldTrials(
      bool has_seed,
      base::FeatureList* feature_list) override;

 private:
  // Instantiates dynamic trials by querying their state, to ensure they get
  // reported as used.
  void InstantiateDynamicTrials();

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserFieldTrials);
};

#endif  // CHROME_BROWSER_CHROME_BROWSER_FIELD_TRIALS_H_
