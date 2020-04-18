// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_ANDROID_AFFILIATION_AFFILIATION_FETCHER_DELEGATE_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_ANDROID_AFFILIATION_AFFILIATION_FETCHER_DELEGATE_H_

#include <memory>
#include <vector>

#include "components/password_manager/core/browser/android_affiliation/affiliation_utils.h"

namespace password_manager {

// Interface that users of AffiliationFetcher should implement to get results of
// the fetch. It is safe to destroy the fetcher in any of the event handlers.
class AffiliationFetcherDelegate {
 public:
  // Encapsulates the response to an affiliations request.
  typedef std::vector<AffiliatedFacets> Result;

  // Called when affiliation information has been successfully retrieved. The
  // |result| will contain at most as many equivalence class as facet URIs in
  // the request, and each requested facet URI will appear in exactly one
  // equivalence class.
  virtual void OnFetchSucceeded(std::unique_ptr<Result> result) = 0;

  // Called when affiliation information could not be fetched due to a network
  // error or a presumably transient server error. The implementor may and will
  // probably want to retry the request (once network connectivity is
  // re-established, and/or with exponential back-off).
  virtual void OnFetchFailed() = 0;

  // Called when an affiliation response was received, but it was either gravely
  // ill-formed or self-inconsistent. It is likely that a repeated fetch would
  // yield the same, erroneous response, therefore, to avoid overloading the
  // server, the fetch must not be repeated in the short run.
  virtual void OnMalformedResponse() = 0;

 protected:
  virtual ~AffiliationFetcherDelegate() {}
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_ANDROID_AFFILIATION_AFFILIATION_FETCHER_DELEGATE_H_
