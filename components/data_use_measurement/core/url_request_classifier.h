// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_USE_MEASUREMENT_CORE_URL_REQUEST_CLASSIFIER_H_
#define COMPONENTS_DATA_USE_MEASUREMENT_CORE_URL_REQUEST_CLASSIFIER_H_

#include <stdint.h>

#include "components/data_use_measurement/core/data_use_user_data.h"

namespace net {
class HttpResponseHeaders;
class URLRequest;
}

namespace data_use_measurement {

// Interface for a classifier that can classify URL requests.
class URLRequestClassifier {
 public:
  virtual ~URLRequestClassifier() {}

  // Returns true if the URLRequest |request| is initiated by user traffic.
  virtual bool IsUserRequest(const net::URLRequest& request) const = 0;

  // Returns the content type of the URL request |request| with response headers
  // |response_headers|. |is_app_foreground| and |is_tab_visible| indicate the
  // current app and tab visibility state.
  virtual DataUseUserData::DataUseContentType GetContentType(
      const net::URLRequest& request,
      const net::HttpResponseHeaders& response_headers) const = 0;

  // Records the page transition histograms.
  virtual void RecordPageTransitionUMA(uint64_t page_transition,
                                       int64_t received_bytes) const = 0;

  // Returns true if |request| is fetching a favicon.
  virtual bool IsFavIconRequest(const net::URLRequest& request) const = 0;
};

}  // namespace data_use_measurement

#endif  // COMPONENTS_DATA_USE_MEASUREMENT_CORE_URL_REQUEST_CLASSIFIER_H_
