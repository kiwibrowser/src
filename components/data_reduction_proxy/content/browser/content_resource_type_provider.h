// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_CONTENT_RESOURCE_TYPE_PROVIDER_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_CONTENT_RESOURCE_TYPE_PROVIDER_H_

#include "base/containers/mru_cache.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/data_reduction_proxy/core/common/resource_type_provider.h"

class GURL;

namespace net {
class URLRequest;
}

namespace data_reduction_proxy {

// Implements ResourceTypeProvider by maintaining a map from each request's URL
// to its last known content type.
class ContentResourceTypeProvider : public ResourceTypeProvider {
 public:
  ContentResourceTypeProvider();
  ~ContentResourceTypeProvider() override;

 private:
  // ResourceTypeProvider:
  void SetContentType(const net::URLRequest& request) override;
  ResourceTypeProvider::ContentType GetContentType(
      const GURL& url) const override;

  // Map that evicts entries lazily based on the recency of being added.
  base::MRUCache<std::string, ResourceTypeProvider::ContentType>
      resource_content_types_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(ContentResourceTypeProvider);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_CONTENT_RESOURCE_TYPE_PROVIDER_H_
