// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_USE_COUNTER_UKM_FEATURES_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_USE_COUNTER_UKM_FEATURES_H_

#include "third_party/blink/public/platform/web_feature.mojom.h"

// This file defines a list of UseCounter WebFeature measured in the
// UKM-based UseCounter. Features must all satisfy UKM privacy requirements
// (see go/ukm). In addition, features should only be added if it's shown
// (or highly likely be) rare, e.g. <1% of page views as measured by UMA.
//
// UKM-based UseCounter should be used to cover the case when UMA UseCounter
// data shows a behaviour that is rare but too common to bindly change.
// UKM-based UseCounter would allow use to find specific pages to reason about
// either a breaking change is acceptable or not.

// Returns true if a given feature is an opt-in UKM feature for use counter.
bool IsAllowedUkmFeature(blink::mojom::WebFeature feature);

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_USE_COUNTER_UKM_FEATURES_H_
