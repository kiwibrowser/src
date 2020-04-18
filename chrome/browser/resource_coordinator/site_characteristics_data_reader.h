// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_SITE_CHARACTERISTICS_DATA_READER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_SITE_CHARACTERISTICS_DATA_READER_H_

#include "chrome/browser/resource_coordinator/local_site_characteristics_feature_usage.h"

namespace resource_coordinator {

// Pure virtual interface to read the characteristics of an origin. This is a
// usable abstraction for both the local and global database.
class SiteCharacteristicsDataReader {
 public:
  SiteCharacteristicsDataReader() = default;
  virtual ~SiteCharacteristicsDataReader() {}

  // Accessors for the site characteristics usage.
  virtual SiteFeatureUsage UpdatesFaviconInBackground() const = 0;
  virtual SiteFeatureUsage UpdatesTitleInBackground() const = 0;
  virtual SiteFeatureUsage UsesAudioInBackground() const = 0;
  virtual SiteFeatureUsage UsesNotificationsInBackground() const = 0;
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_SITE_CHARACTERISTICS_DATA_READER_H_
