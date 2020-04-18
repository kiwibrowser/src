// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_RESOURCE_TYPE_PROVIDER_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_RESOURCE_TYPE_PROVIDER_H_

#include "base/macros.h"

class GURL;

namespace net {
class URLRequest;
}

namespace data_reduction_proxy {

// Class responsible for deciding the content type of a request.
class ResourceTypeProvider {
 public:
  // This enum should remain synchronized with the
  // DataReductionProxyResourceContentType enum in histograms.xml.
  enum ContentType {
    CONTENT_TYPE_UNKNOWN = 0,
    CONTENT_TYPE_MEDIA = 1,
    CONTENT_TYPE_MAIN_FRAME = 2,
    CONTENT_TYPE_MAX = 3,
  };
  virtual ~ResourceTypeProvider() {}

  virtual void SetContentType(const net::URLRequest& request) = 0;

  virtual ContentType GetContentType(const GURL& url) const = 0;

 protected:
  ResourceTypeProvider() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ResourceTypeProvider);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_RESOURCE_TYPE_PROVIDER_H_
