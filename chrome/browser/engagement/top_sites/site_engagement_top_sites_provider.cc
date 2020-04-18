// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/engagement/top_sites/site_engagement_top_sites_provider.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/engagement/site_engagement_service.h"

class SiteEngagementTopSitesProvider::HistoryQuery {
 public:
  HistoryQuery(
      SiteEngagementTopSitesProvider* provider,
      history::HistoryService* history_service,
      const history::HistoryService::QueryMostVisitedURLsCallback& callback)
      : provider_(provider),
        history_service_(history_service),
        callback_(callback),
        pending_history_queries_(0),
        weak_ptr_factory_(this) {}

  void FillMostVisitedURLDetails(base::CancelableTaskTracker* tracker) {
    if (urls_.empty()) {
      callback_.Run(&urls_);
      provider_->OnHistoryQueryComplete(this);
      return;
    }

    // We make 2 calls to the history service for each URL.
    pending_history_queries_ = urls_.size() * 2;

    for (history::MostVisitedURL& mv : urls_) {
      history_service_->QueryURL(
          mv.url, false,
          base::BindOnce(&HistoryQuery::OnURLQueried,
                         weak_ptr_factory_.GetWeakPtr(), base::Unretained(&mv)),
          tracker);
      history_service_->QueryRedirectsFrom(
          mv.url,
          base::Bind(&HistoryQuery::OnRedirectsQueried,
                     weak_ptr_factory_.GetWeakPtr(), base::Unretained(&mv)),
          tracker);
    }
  }

  void AddURL(const history::MostVisitedURL& mv) { urls_.push_back(mv); }
  int NumURLs() const { return urls_.size(); }

 private:
  void OnURLQueried(history::MostVisitedURL* mv,
                    bool success,
                    const history::URLRow& row,
                    const history::VisitVector& visits) {
    DCHECK_GT(pending_history_queries_, 0);
    --pending_history_queries_;

    if (success)
      mv->title = row.title();

    if (pending_history_queries_ == 0) {
      callback_.Run(&urls_);
      provider_->OnHistoryQueryComplete(this);
    }
  }

  void OnRedirectsQueried(history::MostVisitedURL* mv,
                          const history::RedirectList* redirects) {
    DCHECK_GT(pending_history_queries_, 0);
    --pending_history_queries_;

    mv->InitRedirects(*redirects);

    if (pending_history_queries_ == 0) {
      callback_.Run(&urls_);
      provider_->OnHistoryQueryComplete(this);
    }
  }

  SiteEngagementTopSitesProvider* provider_;
  history::HistoryService* history_service_;
  history::MostVisitedURLList urls_;
  history::HistoryService::QueryMostVisitedURLsCallback callback_;

  int pending_history_queries_;
  base::WeakPtrFactory<SiteEngagementTopSitesProvider::HistoryQuery>
      weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(HistoryQuery);
};

SiteEngagementTopSitesProvider::SiteEngagementTopSitesProvider(
    SiteEngagementService* engagement_service,
    history::HistoryService* history_service)
    : engagement_service_(engagement_service),
      history_service_(history_service) {
  DCHECK(engagement_service);
  DCHECK(history_service);
}

SiteEngagementTopSitesProvider::~SiteEngagementTopSitesProvider() {}

void SiteEngagementTopSitesProvider::ProvideTopSites(
    int result_count,
    const history::HistoryService::QueryMostVisitedURLsCallback& callback,
    base::CancelableTaskTracker* tracker) {
  auto request =
      std::make_unique<HistoryQuery>(this, history_service_, callback);

  std::vector<mojom::SiteEngagementDetails> details =
      engagement_service_->GetAllDetails();
  std::sort(details.begin(), details.end(),
            [](const mojom::SiteEngagementDetails& lhs,
               const mojom::SiteEngagementDetails& rhs) {
              return lhs.total_score > rhs.total_score;
            });

  for (const auto& detail : details) {
    if (request->NumURLs() >= result_count)
      break;

    // The default TopSites provider handles sites on paths; for now, we only
    // provide origins from site engagement. The rest of the fields will be
    // filled in by querying the history service.
    history::MostVisitedURL mv;
    mv.url = detail.origin;
    request->AddURL(mv);
  }

  // The request object is responsible for tracking the queries to the history
  // service. It will be deleted once all the queries are completed.
  requests_.push_back(std::move(request));
  requests_.back()->FillMostVisitedURLDetails(tracker);
}

void SiteEngagementTopSitesProvider::OnHistoryQueryComplete(
    const HistoryQuery* request) {
  auto it = std::find_if(requests_.begin(), requests_.end(),
                         [request](const std::unique_ptr<HistoryQuery>& r) {
                           return r.get() == request;
                         });

  DCHECK(it != requests_.end());
  requests_.erase(it);
}
