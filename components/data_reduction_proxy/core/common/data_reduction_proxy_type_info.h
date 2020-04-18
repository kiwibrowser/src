// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_TYPE_INFO_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_TYPE_INFO_H_

#include <stddef.h>

#include <vector>

namespace data_reduction_proxy {

class DataReductionProxyServer;

// Contains information about a given proxy server.
struct DataReductionProxyTypeInfo {
  DataReductionProxyTypeInfo(
      const std::vector<DataReductionProxyServer>& proxy_servers,
      size_t proxy_index)
      : proxy_servers(proxy_servers), proxy_index(proxy_index) {}

  // The full configured list of proxy servers that includes the target proxy
  // server. Since this is held onto as a const reference, the caller needs to
  // ensure that it doesn't try to access |proxy_servers| if the list of
  // configured proxies has changed since this DataReductionProxyTypeInfo was
  // created.
  const std::vector<DataReductionProxyServer>& proxy_servers;

  // The index of this proxy in |proxy_servers|.
  size_t proxy_index;
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_TYPE_INFO_H_
