// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_ANDROID_AFFILIATION_TEST_AFFILIATION_FETCHER_FACTORY_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_ANDROID_AFFILIATION_TEST_AFFILIATION_FETCHER_FACTORY_H_

#include <vector>

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace password_manager {

class FacetURI;
class AffiliationFetcherDelegate;

// Interface for a factory to be used by AffiliationFetcher::Create() in tests
// to construct instances of test-specific AffiliationFetcher subclasses.
//
// The factory is registered with AffiliationFetcher::SetFactoryForTesting().
class TestAffiliationFetcherFactory {
 public:
  // Constructs a fetcher to retrieve affiliations for each facet in |facet_ids|
  // using the specified |request_context_getter|, and will provide the results
  // to the |delegate| on the same thread that creates the instance.
  virtual AffiliationFetcher* CreateInstance(
      net::URLRequestContextGetter* request_context_getter,
      const std::vector<FacetURI>& facet_ids,
      AffiliationFetcherDelegate* delegate) = 0;

 protected:
  virtual ~TestAffiliationFetcherFactory() {}
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_ANDROID_AFFILIATION_TEST_AFFILIATION_FETCHER_FACTORY_H_
