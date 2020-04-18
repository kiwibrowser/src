// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CORE_PREVIEWS_FEATURES_H_
#define COMPONENTS_PREVIEWS_CORE_PREVIEWS_FEATURES_H_

#include "base/feature_list.h"

namespace previews {
namespace features {

extern const base::Feature kPreviews;
extern const base::Feature kOfflinePreviews;
extern const base::Feature kClientLoFi;
extern const base::Feature kNoScriptPreviews;
extern const base::Feature kStalePreviewsTimestamp;
extern const base::Feature kAMPRedirection;
extern const base::Feature kOptimizationHints;

}  // namespace features
}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CORE_PREVIEWS_FEATURES_H_
