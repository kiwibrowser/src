// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/android_affiliation/fake_affiliation_fetcher.h"

#include <utility>

namespace password_manager {

password_manager::FakeAffiliationFetcher::FakeAffiliationFetcher(
    net::URLRequestContextGetter* request_context_getter,
    const std::vector<FacetURI>& facet_ids,
    AffiliationFetcherDelegate* delegate)
    : AffiliationFetcher(request_context_getter, facet_ids, delegate) {
}

password_manager::FakeAffiliationFetcher::~FakeAffiliationFetcher() {
}

void password_manager::FakeAffiliationFetcher::SimulateSuccess(
    std::unique_ptr<AffiliationFetcherDelegate::Result> fake_result) {
  delegate()->OnFetchSucceeded(std::move(fake_result));
}

void password_manager::FakeAffiliationFetcher::SimulateFailure() {
  delegate()->OnFetchFailed();
}

void password_manager::FakeAffiliationFetcher::StartRequest() {
  // Fake. Does nothing.
}

password_manager::ScopedFakeAffiliationFetcherFactory::
    ScopedFakeAffiliationFetcherFactory() {
  AffiliationFetcher::SetFactoryForTesting(this);
}

password_manager::ScopedFakeAffiliationFetcherFactory::
    ~ScopedFakeAffiliationFetcherFactory() {
  AffiliationFetcher::SetFactoryForTesting(nullptr);
}

FakeAffiliationFetcher* ScopedFakeAffiliationFetcherFactory::PopNextFetcher() {
  DCHECK(!pending_fetchers_.empty());
  FakeAffiliationFetcher* first = pending_fetchers_.front();
  pending_fetchers_.pop();
  return first;
}

FakeAffiliationFetcher* ScopedFakeAffiliationFetcherFactory::PeekNextFetcher() {
  DCHECK(!pending_fetchers_.empty());
  return pending_fetchers_.front();
}

AffiliationFetcher* ScopedFakeAffiliationFetcherFactory::CreateInstance(
    net::URLRequestContextGetter* request_context_getter,
    const std::vector<FacetURI>& facet_ids,
    AffiliationFetcherDelegate* delegate) {
  FakeAffiliationFetcher* fetcher =
      new FakeAffiliationFetcher(request_context_getter, facet_ids, delegate);
  pending_fetchers_.push(fetcher);
  return fetcher;
}

}  // namespace password_manager
