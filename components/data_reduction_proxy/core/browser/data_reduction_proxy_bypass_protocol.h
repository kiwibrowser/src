// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_BYPASS_PROTOCOL_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_BYPASS_PROTOCOL_H_

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_headers.h"

namespace net {
class URLRequest;
}

namespace data_reduction_proxy {

class DataReductionProxyConfig;

// Class responsible for determining when a response should or should not cause
// the data reduction proxy to be bypassed, and to what degree. Owned by the
// DataReductionProxyInterceptor.
class DataReductionProxyBypassProtocol {
 public:
  // Constructs a DataReductionProxyBypassProtocol object. |config| must be
  // non-NULL and outlive |this|.
  DataReductionProxyBypassProtocol(DataReductionProxyConfig* config);

  ~DataReductionProxyBypassProtocol();

  // Decides whether to mark the data reduction proxy as temporarily bad and
  // put it on the proxy retry map, which is maintained by the
  // ProxyResolutionService of the URLRequestContext. Returns true if the
  // request should be retried. Updates the load flags in |request| for some
  // bypass types, e.g., "block-once". Returns the DataReductionProxyBypassType
  // (if not NULL).
  bool MaybeBypassProxyAndPrepareToRetry(
      net::URLRequest* request,
      DataReductionProxyBypassType* proxy_bypass_type,
      DataReductionProxyInfo* data_reduction_proxy_info);

 private:
  // Decides whether to mark the data reduction proxy as temporarily bad and
  // put it on the proxy retry map. Returns true if the request should be
  // retried. Should be called only when the response of the |request| had null
  // response headers.
  bool HandleInvalidResponseHeadersCase(
      const net::URLRequest& request,
      DataReductionProxyInfo* data_reduction_proxy_info,
      DataReductionProxyBypassType* bypass_type) const;

  // Decides whether to mark the data reduction proxy as temporarily bad and
  // put it on the proxy retry map. Returns true if the request should be
  // retried. Should be called only when the response of the |request| had
  // non-null response headers.
  bool HandleValidResponseHeadersCase(
      const net::URLRequest& request,
      DataReductionProxyBypassType* proxy_bypass_type,
      DataReductionProxyInfo* data_reduction_proxy_info,
      DataReductionProxyBypassType* bypass_type) const;

  // Must outlive |this|.
  DataReductionProxyConfig* config_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyBypassProtocol);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_BYPASS_PROTOCOL_H_
