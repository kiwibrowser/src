// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_PROVIDER_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_PROVIDER_H_

#include "components/history/core/browser/history_service.h"

namespace history {

// Interface for querying the URLs that should be included in TopSites.
class TopSitesProvider {
 public:
  virtual ~TopSitesProvider() {}

  // Asynchronously queries for the list of URLs to include in TopSites, running
  // |callback| when complete. |tracker| can be used to interrupt an
  // in-progress call.
  virtual void ProvideTopSites(
      int result_count,
      const HistoryService::QueryMostVisitedURLsCallback& callback,
      base::CancelableTaskTracker* tracker) = 0;
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_PROVIDER_H_
