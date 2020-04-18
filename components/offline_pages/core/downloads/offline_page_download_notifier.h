// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_DOWNLOADS_OFFLINE_PAGE_DOWNLOAD_NOTIFIER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_DOWNLOADS_OFFLINE_PAGE_DOWNLOAD_NOTIFIER_H_

#include "components/offline_items_collection/core/offline_item.h"

using OfflineItem = offline_items_collection::OfflineItem;

namespace offline_pages {

struct OfflinePageDownloadNotifier {
 public:
  virtual ~OfflinePageDownloadNotifier() = default;

  // Reports that |item| has completed successfully.
  virtual void NotifyDownloadSuccessful(const OfflineItem& item) = 0;

  // Reports that |item| has completed unsuccessfully.
  virtual void NotifyDownloadFailed(const OfflineItem& item) = 0;

  // Reports that |item| is active and possibly making progress.
  virtual void NotifyDownloadProgress(const OfflineItem& item) = 0;

  // Reports that |item| has been paused (and so it is not active).
  virtual void NotifyDownloadPaused(const OfflineItem& item) = 0;

  // Reports that any progress on |item| has been interrupted. It is pending
  // or available for another attempt when conditions allows.
  virtual void NotifyDownloadInterrupted(const OfflineItem& item) = 0;

  // Reports that |item| has been canceled.
  virtual void NotifyDownloadCanceled(const OfflineItem& item) = 0;

  // Suppresses the download complete notification
  // depending on flags and origin.
  virtual bool MaybeSuppressNotification(const std::string& origin,
                                         const OfflineItem& item) = 0;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_DOWNLOADS_OFFLINE_PAGE_DOWNLOAD_NOTIFIER_H_
