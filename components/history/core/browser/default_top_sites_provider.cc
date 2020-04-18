// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/core/browser/default_top_sites_provider.h"

namespace {

const int kDaysOfHistory = 90;

}  // namespace

namespace history {

DefaultTopSitesProvider::DefaultTopSitesProvider(
    HistoryService* history_service)
    : history_service_(history_service) {}

DefaultTopSitesProvider::~DefaultTopSitesProvider() {}

void DefaultTopSitesProvider::ProvideTopSites(
    int result_count,
    const HistoryService::QueryMostVisitedURLsCallback& callback,
    base::CancelableTaskTracker* tracker) {
  history_service_->QueryMostVisitedURLs(result_count, kDaysOfHistory, callback,
                                         tracker);
}

}  // namespace history
