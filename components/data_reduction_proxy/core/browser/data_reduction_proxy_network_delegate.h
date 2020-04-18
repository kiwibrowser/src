// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_NETWORK_DELEGATE_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_NETWORK_DELEGATE_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_metrics.h"
#include "net/base/completion_callback.h"
#include "net/base/layered_network_delegate.h"
#include "net/proxy_resolution/proxy_retry_info.h"

class GURL;

namespace net {
class HttpRequestHeaders;
class NetworkDelegate;
class ProxyConfig;
class ProxyInfo;
class URLRequest;
}

namespace data_reduction_proxy {
class DataReductionProxyBypassStats;
class DataReductionProxyConfig;
class DataReductionProxyConfigurator;
class DataReductionProxyIOData;
class DataReductionProxyRequestOptions;

// Values of the UMA DataReductionProxy.LoFi.TransformationType histogram.
// This enum must remain synchronized with
// DataReductionProxyLoFiTransformationType in
// metrics/histograms/histograms.xml.
enum LitePageTransformationType {
  LITE_PAGE = 0,
  NO_TRANSFORMATION_LITE_PAGE_REQUESTED,
  LITE_PAGE_TRANSFORMATION_TYPES_INDEX_BOUNDARY,
};

// DataReductionProxyNetworkDelegate is a LayeredNetworkDelegate that wraps a
// NetworkDelegate and adds Data Reduction Proxy specific logic.
class DataReductionProxyNetworkDelegate : public net::LayeredNetworkDelegate {
 public:
  // Provides an additional proxy configuration that can be consulted after
  // proxy resolution. Used to get the Data Reduction Proxy config and give it
  // to the OnResolveProxyHandler and RecordBypassedBytesHistograms.
  typedef base::Callback<const net::ProxyConfig&()> ProxyConfigGetter;

  // Constructs a DataReductionProxyNetworkDelegate object with the given
  // |network_delegate|, |config|, |handler|, |configurator|, and
  // |experiments_stats|. Takes ownership of
  // and wraps the |network_delegate|, calling an internal implementation for
  // each delegate method. For example, the implementation of
  // OnHeadersReceived() calls OnHeadersReceivedInternal().
  DataReductionProxyNetworkDelegate(
      std::unique_ptr<net::NetworkDelegate> network_delegate,
      DataReductionProxyConfig* config,
      DataReductionProxyRequestOptions* handler,
      const DataReductionProxyConfigurator* configurator);
  ~DataReductionProxyNetworkDelegate() override;

  // Initializes member variables to record data reduction proxy prefs and
  // report UMA.
  void InitIODataAndUMA(
      DataReductionProxyIOData* io_data,
      DataReductionProxyBypassStats* bypass_stats);

 private:
  friend class DataReductionProxyTestContext;

  // Resets if Lo-Fi has been used for the last main frame load to false.
  void OnBeforeURLRequestInternal(net::URLRequest* request,
                                  const net::CompletionCallback& callback,
                                  GURL* new_url) override;

  // Called before an HTTP transaction is started. Allows the delegate to
  // modify the Chrome-Proxy-Accept-Transform header to convey acceptable
  // content transformations.
  void OnBeforeStartTransactionInternal(
      net::URLRequest* request,
      const net::CompletionCallback& callback,
      net::HttpRequestHeaders* headers) override;

  // Called after connection. Allows the delegate to read/write
  // |headers| before they get sent out. |headers| is valid only until
  // OnCompleted or OnURLRequestDestroyed is called for this request.
  void OnBeforeSendHeadersInternal(
      net::URLRequest* request,
      const net::ProxyInfo& proxy_info,
      const net::ProxyRetryInfoMap& proxy_retry_info,
      net::HttpRequestHeaders* headers) override;

  // Called after a redirect response. Clears out persistent
  // DataReductionProxyData from the URLRequest.
  void OnBeforeRedirectInternal(net::URLRequest* request,
                                const GURL& new_location) override;

  // Indicates that the URL request has been completed or failed.
  // |started| indicates whether the request has been started. If false,
  // some information like the socket address is not available.
  void OnCompletedInternal(net::URLRequest* request,
                           bool started) override;

  // Checks if a LoFi or Lite Pages response was received and sets the state on
  // DataReductionProxyData for |request|.
  void OnHeadersReceivedInternal(
      net::URLRequest* request,
      const net::CompletionCallback& callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;

  // Calculates actual data usage that went over the network at the HTTP layer
  // (e.g. not including network layer overhead) and estimates original data
  // usage for |request|.
  void CalculateAndRecordDataUsage(const net::URLRequest& request,
                                   DataReductionProxyRequestType request_type);

  // Posts to the UI thread to UpdateContentLengthPrefs in the data reduction
  // proxy metrics and updates |received_content_length_| and
  // |original_content_length_|.
  void AccumulateDataUsage(int64_t data_used,
                           int64_t original_size,
                           DataReductionProxyRequestType request_type,
                           const std::string& mime_type);

  // Record information such as histograms related to the Content-Length of
  // |request|. |original_content_length| is the length of the resource if
  // fetched over a direct connection without the Data Reduction Proxy, or -1 if
  // no original content length is available.
  void RecordContentLength(const net::URLRequest& request,
                           DataReductionProxyRequestType request_type,
                           int64_t original_content_length);

  // Records UMA that counts how many pages were transformed by various lite
  // page transformations.
  void RecordLitePageTransformationType(LitePageTransformationType type);

  // Returns whether |request| would have used the data reduction proxy server
  // if the holdback fieldtrial weren't enabled. |proxy_info| is the list of
  // proxies being used, and |proxy_retry_info| contains a list of bad proxies.
  bool WasEligibleWithoutHoldback(
      const net::URLRequest& request,
      const net::ProxyInfo& proxy_info,
      const net::ProxyRetryInfoMap& proxy_retry_info) const;

  // May add Brotli to Accept Encoding request header if |proxy_info| contains
  // a proxy server that is expected to support Brotli encoding.
  void MaybeAddBrotliToAcceptEncodingHeader(
      const net::ProxyInfo& proxy_info,
      net::HttpRequestHeaders* request_headers,
      const net::URLRequest& request) const;

  // May add chrome-proxy-ect header to |request_headers| if adding of
  // chrome-proxy-ect is enabled via field trial and a valid estimate of
  // network quality is available. This method should be called only when the
  // resolved proxy for |request| is a data saver proxy.
  void MaybeAddChromeProxyECTHeader(net::HttpRequestHeaders* request_headers,
                                    const net::URLRequest& request) const;

  // Removes the chrome-proxy-ect header from |request_headers|.
  void RemoveChromeProxyECTHeader(
      net::HttpRequestHeaders* request_headers) const;

  // All raw Data Reduction Proxy pointers must outlive |this|.
  DataReductionProxyConfig* data_reduction_proxy_config_;

  DataReductionProxyBypassStats* data_reduction_proxy_bypass_stats_;

  DataReductionProxyRequestOptions* data_reduction_proxy_request_options_;

  DataReductionProxyIOData* data_reduction_proxy_io_data_;

  const DataReductionProxyConfigurator* configurator_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyNetworkDelegate);
};
}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_NETWORK_DELEGATE_H_
