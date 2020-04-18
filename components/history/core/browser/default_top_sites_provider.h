// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_DEFAULT_TOP_SITES_PROVIDER_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_DEFAULT_TOP_SITES_PROVIDER_H_

#include "base/macros.h"
#include "components/history/core/browser/top_sites_provider.h"

namespace history {

// Queries the history service for most frequently recently visited sites and
// provides them for TopSites.
class DefaultTopSitesProvider : public TopSitesProvider {
 public:
  DefaultTopSitesProvider(HistoryService* history_service);
  ~DefaultTopSitesProvider() override;

  void ProvideTopSites(
      int result_count,
      const HistoryService::QueryMostVisitedURLsCallback& callback,
      base::CancelableTaskTracker* tracker) override;

 private:
  HistoryService* history_service_;

  DISALLOW_COPY_AND_ASSIGN(DefaultTopSitesProvider);
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_DEFAULT_TOP_SITES_PROVIDER_H_
