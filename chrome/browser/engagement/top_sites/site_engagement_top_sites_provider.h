// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ENGAGEMENT_TOP_SITES_SITE_ENGAGEMENT_TOP_SITES_PROVIDER_H_
#define CHROME_BROWSER_ENGAGEMENT_TOP_SITES_SITE_ENGAGEMENT_TOP_SITES_PROVIDER_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/top_sites_provider.h"

class SiteEngagementService;

// Provides the top-ranked origins by engagement for TopSites, querying the
// history service for title and redirect information.
// TODO(crbug.com/775390): move this into TopSites once site engagement is
// componentized.
class SiteEngagementTopSitesProvider : public history::TopSitesProvider {
 public:
  SiteEngagementTopSitesProvider(SiteEngagementService* engagement_service,
                                 history::HistoryService* history_service);
  ~SiteEngagementTopSitesProvider() override;

  void ProvideTopSites(
      int result_count,
      const history::HistoryService::QueryMostVisitedURLsCallback& callback,
      base::CancelableTaskTracker* tracker) override;

 private:
  // This represents a set of queries to the history service in response to
  // a call to ProvideTopSites().
  class HistoryQuery;

  void OnHistoryQueryComplete(const HistoryQuery* request);

  SiteEngagementService* engagement_service_;
  history::HistoryService* history_service_;
  std::vector<std::unique_ptr<HistoryQuery>> requests_;

  DISALLOW_COPY_AND_ASSIGN(SiteEngagementTopSitesProvider);
};

#endif  // CHROME_BROWSER_ENGAGEMENT_TOP_SITES_SITE_ENGAGEMENT_TOP_SITES_PROVIDER_H_
