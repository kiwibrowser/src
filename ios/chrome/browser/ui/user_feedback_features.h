// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_USER_FEEDBACK_FEATURES_H_
#define IOS_CHROME_BROWSER_UI_USER_FEEDBACK_FEATURES_H_

#include "base/feature_list.h"

// Feature flag to enable FeedbackKit V2
extern const base::Feature kFeedbackKitV2;

// Feature flag to send SSOService to FeedbackKit V2. This feature flag is
// used only if kFeedbackKitV2 is enabled.
extern const base::Feature kFeedbackKitV2WithSSOService;

#endif  // IOS_CHROME_BROWSER_UI_USER_FEEDBACK_FEATURES_H_
