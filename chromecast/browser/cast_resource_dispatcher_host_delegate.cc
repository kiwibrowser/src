// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_resource_dispatcher_host_delegate.h"

#include "chromecast/base/metrics/cast_metrics_helper.h"
#include "chromecast/browser/cast_browser_process.h"
#include "chromecast/net/connectivity_checker.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_status.h"

namespace chromecast {
namespace shell {

void CastResourceDispatcherHostDelegate::RequestComplete(
    net::URLRequest* url_request,
    int net_error) {
  if (net_error == net::OK || net_error == net::ERR_IO_PENDING ||
      net_error == net::ERR_ABORTED)
    return;

  metrics::CastMetricsHelper* metrics_helper =
      metrics::CastMetricsHelper::GetInstance();
  metrics_helper->RecordApplicationEventWithValue(
      "Cast.Platform.ResourceRequestError", url_request->status().error());
  LOG(ERROR) << "Failed to load resource " << url_request->url()
             << ", error:" << net::ErrorToShortString(net_error);
  CastBrowserProcess::GetInstance()->connectivity_checker()->Check();
}

void CastResourceDispatcherHostDelegate::RequestComplete(
    net::URLRequest* url_request) {
  if (url_request->status().status() == net::URLRequestStatus::FAILED) {
    metrics::CastMetricsHelper* metrics_helper =
        metrics::CastMetricsHelper::GetInstance();
    metrics_helper->RecordApplicationEventWithValue(
        "Cast.Platform.ResourceRequestError",
        url_request->status().error());
    LOG(ERROR) << "Failed to load resource " << url_request->url()
               << "; status:" << url_request->status().status() << ", error:"
               << net::ErrorToShortString(url_request->status().error());
    CastBrowserProcess::GetInstance()->connectivity_checker()->Check();
  }
}

}  // namespace shell
}  // namespace chromecast
