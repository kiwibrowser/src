// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/android_affiliation/fake_affiliation_api.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "testing/gtest/include/gtest/gtest.h"

namespace password_manager {

ScopedFakeAffiliationAPI::ScopedFakeAffiliationAPI() {
}

ScopedFakeAffiliationAPI::~ScopedFakeAffiliationAPI() {
  // Note that trying to provide details of dangling fetchers would be unwise,
  // as it is quite possible that they have been destroyed already.
  EXPECT_FALSE(HasPendingRequest())
      << "Pending AffilitionFetcher on shutdown.\n"
      << "Call IgnoreNextRequest() if this is intended.";
}

void ScopedFakeAffiliationAPI::AddTestEquivalenceClass(
    const AffiliatedFacets& affiliated_facets) {
  preset_equivalence_relation_.push_back(affiliated_facets);
}

bool ScopedFakeAffiliationAPI::HasPendingRequest() {
  return fake_fetcher_factory_.has_pending_fetchers();
}

std::vector<FacetURI> ScopedFakeAffiliationAPI::GetNextRequestedFacets() {
  if (fake_fetcher_factory_.has_pending_fetchers())
    return fake_fetcher_factory_.PeekNextFetcher()->requested_facet_uris();
  return std::vector<FacetURI>();
}

void ScopedFakeAffiliationAPI::ServeNextRequest() {
  if (!fake_fetcher_factory_.has_pending_fetchers())
    return;

  FakeAffiliationFetcher* fetcher = fake_fetcher_factory_.PopNextFetcher();
  std::unique_ptr<AffiliationFetcherDelegate::Result> fake_response(
      new AffiliationFetcherDelegate::Result);
  for (const auto& preset_equivalence_class : preset_equivalence_relation_) {
    bool had_intersection_with_request = false;
    for (const auto& requested_facet_uri : fetcher->requested_facet_uris()) {
      if (std::any_of(preset_equivalence_class.begin(),
                      preset_equivalence_class.end(),
                      [&requested_facet_uri](const Facet& facet) {
                        return facet.uri == requested_facet_uri;
                      })) {
        had_intersection_with_request = true;
        break;
      }
    }
    if (had_intersection_with_request)
      fake_response->push_back(preset_equivalence_class);
  }
  fetcher->SimulateSuccess(std::move(fake_response));
}

void ScopedFakeAffiliationAPI::FailNextRequest() {
  if (!fake_fetcher_factory_.has_pending_fetchers())
    return;

  FakeAffiliationFetcher* fetcher = fake_fetcher_factory_.PopNextFetcher();
  fetcher->SimulateFailure();
}

void ScopedFakeAffiliationAPI::IgnoreNextRequest() {
  if (!fake_fetcher_factory_.has_pending_fetchers())
    return;
  ignore_result(fake_fetcher_factory_.PopNextFetcher());
}

}  // namespace password_manager
