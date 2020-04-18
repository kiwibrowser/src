// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_DB_V4_FEATURE_LIST_H_
#define COMPONENTS_SAFE_BROWSING_DB_V4_FEATURE_LIST_H_

namespace safe_browsing {

// Exposes methods to check whether a particular feature has been enabled
// through Finch.
namespace V4FeatureList {

enum class V4UsageStatus {
  // The V4 database manager is not even instantiated i.e. is diabled. All
  // SafeBrowsing operations use PVer3 code.
  V4_DISABLED,

  // The V4 database manager is instantiated, and performs background updates,
  // but all SafeBrowsing verdicts are returned using the PVer3 database.
  V4_INSTANTIATED,

  // Only the V4 database manager is instantiated, PVer3 database manager is
  // not. All SafeBrowsing verdicts are returned using PVer4 database.
  V4_ONLY
};

V4UsageStatus GetV4UsageStatus();

}  // namespace V4FeatureList

}  // namespace safe_browsing

#endif  // COMPONENTS_SAFE_BROWSING_DB_V4_FEATURE_LIST_H_
