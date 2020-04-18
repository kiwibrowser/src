// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_SITE_AFFILIATION_ASSET_LINK_RETRIEVER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_SITE_AFFILIATION_ASSET_LINK_RETRIEVER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/password_manager/core/browser/site_affiliation/asset_link_data.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace net {
class URLRequestContextGetter;
}

namespace password_manager {

// The class is responsible for fetching and parsing the digit asset links file.
// The file is a JSON containing different directives for the domain. The class
// is only interested in those related to credentials delegations.
// The spec is
// https://github.com/google/digitalassetlinks/blob/master/well-known/details.md
class AssetLinkRetriever : public base::RefCounted<AssetLinkRetriever>,
                           public net::URLFetcherDelegate {
 public:
  enum class State {
    INACTIVE,
    NETWORK_REQUEST,
    PARSING,
    FINISHED,
  };

  explicit AssetLinkRetriever(GURL file_url);

  // Starts a network request if the current state is INACTIVE. All the calls
  // afterwards are ignored.
  void Start(net::URLRequestContextGetter* context_getter);

  State state() const { return state_; }

  bool error() const { return error_; }

  const std::vector<GURL>& includes() const { return data_.includes(); }
  const std::vector<GURL>& targets() const { return data_.targets(); }

 private:
  friend class base::RefCounted<AssetLinkRetriever>;
  ~AssetLinkRetriever() override;

  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  void OnResponseParsed(std::unique_ptr<AssetLinkData> data, bool result);

  // URL of the file retrieved.
  const GURL url_;

  // Current state of the retrieval.
  State state_;

  // Whether the reading finished with error.
  bool error_;

  // Actual data from the asset link.
  AssetLinkData data_;

  std::unique_ptr<net::URLFetcher> fetcher_;

  DISALLOW_COPY_AND_ASSIGN(AssetLinkRetriever);
};

}  // namespace password_manager
#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_SITE_AFFILIATION_ASSET_LINK_RETRIEVER_H_
