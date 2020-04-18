// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_USE_MEASUREMENT_CONTENT_CONTENT_URL_REQUEST_CLASSIFIER_H_
#define COMPONENTS_DATA_USE_MEASUREMENT_CONTENT_CONTENT_URL_REQUEST_CLASSIFIER_H_

#include "components/data_use_measurement/core/url_request_classifier.h"

namespace net {
class URLRequest;
}

namespace data_use_measurement {

// Returns true if the URLRequest |request| is initiated by user traffic.
bool IsUserRequest(const net::URLRequest& request);

class ContentURLRequestClassifier : public URLRequestClassifier {
 private:
  // UrlRequestClassifier:
  bool IsUserRequest(const net::URLRequest& request) const override;
  DataUseUserData::DataUseContentType GetContentType(
      const net::URLRequest& request,
      const net::HttpResponseHeaders& response_headers) const override;
  void RecordPageTransitionUMA(uint64_t page_transition,
                               int64_t received_bytes) const override;
  bool IsFavIconRequest(const net::URLRequest& request) const override;
};

}  // namespace data_use_measurement

#endif  // COMPONENTS_DATA_USE_MEASUREMENT_CONTENT_CONTENT_URL_REQUEST_CLASSIFIER_H_
