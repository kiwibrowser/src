// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_SEARCH_API_SAFE_SEARCH_URL_CHECKER_H_
#define CHROME_BROWSER_SAFE_SEARCH_API_SAFE_SEARCH_URL_CHECKER_H_

#include <stddef.h>

#include <memory>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/mru_cache.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

// This class uses the SafeSearch API to check the SafeSearch classification
// of the content on a given URL and returns the result asynchronously
// via a callback.
class SafeSearchURLChecker {
 public:
  enum class Classification { SAFE, UNSAFE };

  // Returns whether |url| should be blocked. Called from CheckURL.
  using CheckCallback = base::OnceCallback<
      void(const GURL&, Classification classification, bool /* uncertain */)>;

  explicit SafeSearchURLChecker(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const net::NetworkTrafficAnnotationTag& traffic_annotation);
  SafeSearchURLChecker(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      size_t cache_size);
  ~SafeSearchURLChecker();

  // Returns whether |callback| was run synchronously.
  bool CheckURL(const GURL& url, CheckCallback callback);

  void SetCacheTimeoutForTesting(const base::TimeDelta& timeout) {
    cache_timeout_ = timeout;
  }

 private:
  struct Check;
  struct CheckResult {
    CheckResult(Classification classification, bool uncertain);
    Classification classification;
    bool uncertain;
    base::TimeTicks timestamp;
  };
  using CheckList = std::list<std::unique_ptr<Check>>;

  void OnSimpleLoaderComplete(CheckList::iterator it,
                              std::unique_ptr<std::string> response_body);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  const net::NetworkTrafficAnnotationTag traffic_annotation_;

  CheckList checks_in_progress_;

  base::MRUCache<GURL, CheckResult> cache_;
  base::TimeDelta cache_timeout_;

  DISALLOW_COPY_AND_ASSIGN(SafeSearchURLChecker);
};

#endif  // CHROME_BROWSER_SAFE_SEARCH_API_SAFE_SEARCH_URL_CHECKER_H_
