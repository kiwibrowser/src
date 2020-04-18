// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_USE_MEASUREMENT_CORE_DATA_USE_NETWORK_DELEGATE_H_
#define COMPONENTS_DATA_USE_MEASUREMENT_CORE_DATA_USE_NETWORK_DELEGATE_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "components/data_use_measurement/core/data_use_measurement.h"
#include "components/metrics/data_use_tracker.h"
#include "net/base/completion_callback.h"
#include "net/base/layered_network_delegate.h"

namespace net {
class NetworkDelegate;
class URLRequest;
}

namespace data_use_measurement {

class DataUseAscriber;

// Propagates network data use events to the |ascriber_|. This class must be
// set up as a network delegate for the URLRequestContexts whose data use is to
// be tracked.
class DataUseNetworkDelegate : public net::LayeredNetworkDelegate {
 public:
  DataUseNetworkDelegate(
      std::unique_ptr<net::NetworkDelegate> nested_network_delegate,
      DataUseAscriber* ascriber,
      std::unique_ptr<URLRequestClassifier> url_request_classifier,
      const metrics::UpdateUsagePrefCallbackType& metrics_data_use_forwarder);

  ~DataUseNetworkDelegate() override;

  // LayeredNetworkDelegate:
  void OnBeforeURLRequestInternal(net::URLRequest* request,
                                  const net::CompletionCallback& callback,
                                  GURL* new_url) override;

  void OnBeforeRedirectInternal(net::URLRequest* request,
                                const GURL& new_location) override;

  void OnHeadersReceivedInternal(
      net::URLRequest* request,
      const net::CompletionCallback& callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;

  void OnNetworkBytesReceivedInternal(net::URLRequest* request,
                                      int64_t bytes_received) override;

  void OnNetworkBytesSentInternal(net::URLRequest* request,
                                  int64_t bytes_sent) override;

  void OnCompletedInternal(net::URLRequest* request, bool started) override;

  void OnURLRequestDestroyedInternal(net::URLRequest* request) override;

 private:
  DataUseAscriber* ascriber_;

  // Component to report data use UMA.
  data_use_measurement::DataUseMeasurement data_use_measurement_;
};

}  // namespace data_use_measurement

#endif  // COMPONENTS_DATA_USE_MEASUREMENT_CORE_DATA_USE_NETWORK_DELEGATE_H_
