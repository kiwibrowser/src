// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSER_WATCHER_FEATURES_H_
#define COMPONENTS_BROWSER_WATCHER_FEATURES_H_

#include "base/feature_list.h"

namespace browser_watcher {

// Enables recording persistent stability information, which can later be
// collected in the event of an unclean shutdown.
extern const base::Feature kStabilityDebuggingFeature;

// Name of an experiment parameter that controls whether to perform an initial
// flush.
extern const char kInitFlushParam[];

// Name of an experiment parameter that controls whether to collect postmortem
// reports.
extern const char kCollectPostmortemParam[];

}  // namespace browser_watcher

#endif  // COMPONENTS_BROWSER_WATCHER_FEATURES_H_
