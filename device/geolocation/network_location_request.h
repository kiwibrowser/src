// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GEOLOCATION_NETWORK_LOCATION_REQUEST_H_
#define DEVICE_GEOLOCATION_NETWORK_LOCATION_REQUEST_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "device/geolocation/geolocation_export.h"
#include "device/geolocation/wifi_data_provider.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "services/device/public/mojom/geoposition.mojom.h"
#include "url/gurl.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;

struct PartialNetworkTrafficAnnotationTag;
}

namespace device {

// Takes wifi data and sends it to a server to get a position fix.
// It performs formatting of the request and interpretation of the response.
class DEVICE_GEOLOCATION_EXPORT NetworkLocationRequest
    : private net::URLFetcherDelegate {
 public:
  // ID passed to URLFetcher::Create(). Used for testing.
  static int url_fetcher_id_for_tests;

  // Called when a new geo position is available. The second argument indicates
  // whether there was a server error or not. It is true when there was a
  // server or network error - either no response or a 500 error code.
  using LocationResponseCallback =
      base::Callback<void(const mojom::Geoposition& /* position */,
                          bool /* server_error */,
                          const WifiData& /* wifi_data */)>;

  NetworkLocationRequest(scoped_refptr<net::URLRequestContextGetter> context,
                         const std::string& api_key,
                         LocationResponseCallback callback);
  ~NetworkLocationRequest() override;

  // Makes a new request using the specified |wifi_data|. Returns true if the
  // new request was successfully started. In all cases, any currently pending
  // request will be canceled. The specified |wifi_data| and |wifi_timestamp|
  // are passed back to the client upon completion, via
  // LocationResponseCallback's |wifi_data| and |position.timestamp|
  // respectively.
  bool MakeRequest(const WifiData& wifi_data,
                   const base::Time& wifi_timestamp,
                   const net::PartialNetworkTrafficAnnotationTag&
                       partial_traffic_annotation);

  bool is_request_pending() const { return url_fetcher_ != NULL; }

 private:
  // net::URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  const scoped_refptr<net::URLRequestContextGetter> url_context_;
  const std::string api_key_;
  const LocationResponseCallback location_response_callback_;
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  // Keep a copy of the data sent in the request, so we can refer back to it
  // when the response arrives.
  WifiData wifi_data_;
  base::Time wifi_timestamp_;

  // The start time for the request.
  base::TimeTicks request_start_time_;

  DISALLOW_COPY_AND_ASSIGN(NetworkLocationRequest);
};

}  // namespace device

#endif  // DEVICE_GEOLOCATION_NETWORK_LOCATION_REQUEST_H_
