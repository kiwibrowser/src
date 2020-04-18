// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_SITE_CHARACTERISTICS_DATA_WRITER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_SITE_CHARACTERISTICS_DATA_WRITER_H_

namespace resource_coordinator {

// Pure virtual interface to record the observations made for an origin.
class SiteCharacteristicsDataWriter {
 public:
  SiteCharacteristicsDataWriter() = default;
  virtual ~SiteCharacteristicsDataWriter() {}

  // Records tab load/unload events.
  virtual void NotifySiteLoaded() = 0;
  virtual void NotifySiteUnloaded() = 0;

  // Records feature usage.
  virtual void NotifyUpdatesFaviconInBackground() = 0;
  virtual void NotifyUpdatesTitleInBackground() = 0;
  virtual void NotifyUsesAudioInBackground() = 0;
  virtual void NotifyUsesNotificationsInBackground() = 0;
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_SITE_CHARACTERISTICS_DATA_WRITER_H_
