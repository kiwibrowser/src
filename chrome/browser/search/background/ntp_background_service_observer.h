// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_BACKGROUND_NTP_BACKGROUND_SERVICE_OBSERVER_H_
#define CHROME_BROWSER_SEARCH_BACKGROUND_NTP_BACKGROUND_SERVICE_OBSERVER_H_

// Observer for NtpBackgroundService.
class NtpBackgroundServiceObserver {
 public:
  // Called when the CollectionInfo is updated, usually as the result of a
  // FetchCollectionInfo() call on the service. Note that this is called after
  // each FetchCollectionInfo(), even if the network request failed, or if it
  // didn't result in an actual change to the cached data. You can get the new
  // data via NtpBackgroundService::collection_info().
  virtual void OnCollectionInfoAvailable() = 0;

  // Called when the CollectionImages are updated, usually as the result of a
  // FetchCollectionImageInfo() call on the service. Note that this is called
  // after each FetchCollectionImage(), even if the network request failed, or
  // if it didn't result in an actual change to the cached data. You can get the
  // new data via NtpBackgroundService::collection_images().
  virtual void OnCollectionImagesAvailable() = 0;

  // Called when the OnNtpBackgroundService is shutting down. Observers that
  // might outlive the service should use this to unregister themselves, and
  // clear out any pointers to the service they might hold.
  virtual void OnNtpBackgroundServiceShuttingDown() {}
};

#endif  // CHROME_BROWSER_SEARCH_BACKGROUND_NTP_BACKGROUND_SERVICE_OBSERVER_H_
