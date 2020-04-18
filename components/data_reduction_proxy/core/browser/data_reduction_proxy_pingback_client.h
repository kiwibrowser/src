// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_PINGBACK_CLIENT_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_PINGBACK_CLIENT_H_

namespace data_reduction_proxy {
class DataReductionProxyData;
struct DataReductionProxyPageLoadTiming;

// Manages pingbacks about page load timing information to the data saver proxy
// server.
class DataReductionProxyPingbackClient {
 public:
  virtual ~DataReductionProxyPingbackClient(){};

  // Sends a pingback to the data saver proxy server about various timing
  // information.
  virtual void SendPingback(const DataReductionProxyData& data,
                            const DataReductionProxyPageLoadTiming& timing) = 0;

  // Sets the probability of actually sending a pingback to the server for any
  // call to SendPingback.
  virtual void SetPingbackReportingFraction(
      float pingback_reporting_fraction) = 0;
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_PINGBACK_CLIENT_H_
