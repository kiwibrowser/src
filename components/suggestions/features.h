// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUGGESTIONS_FEATURES_H_
#define COMPONENTS_SUGGESTIONS_FEATURES_H_

#include "base/feature_list.h"

namespace suggestions {

// If this feature is enabled, we request and use suggestions even if there are
// only very few of them.
extern const base::Feature kUseSuggestionsEvenIfFewFeature;

}  // namespace suggestions

#endif  // COMPONENTS_SUGGESTIONS_FEATURES_H_
