// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_FEATURE_USAGE_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_FEATURE_USAGE_H_

namespace resource_coordinator {

// A tri-state return value for site feature usage. If a definitive decision
// can't be made then an "unknown" result can be returned.
enum class SiteFeatureUsage {
  kSiteFeatureNotInUse,
  kSiteFeatureInUse,
  kSiteFeatureUsageUnknown,
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_LOCAL_SITE_CHARACTERISTICS_FEATURE_USAGE_H_
