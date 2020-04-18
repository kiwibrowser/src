// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/signed_exchange_utils.h"

#include "base/feature_list.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "content/browser/web_package/signed_exchange_devtools_proxy.h"
#include "content/browser/web_package/web_package_request_handler.h"
#include "content/public/common/content_features.h"
#include "services/network/public/cpp/resource_response.h"
#include "third_party/blink/public/common/origin_trials/trial_token_validator.h"

namespace content {
namespace signed_exchange_utils {

void ReportErrorAndEndTraceEvent(SignedExchangeDevToolsProxy* devtools_proxy,
                                 const char* trace_event_name,
                                 const std::string& error_message) {
  if (devtools_proxy)
    devtools_proxy->ReportErrorMessage(error_message);
  TRACE_EVENT_END1(TRACE_DISABLED_BY_DEFAULT("loading"), trace_event_name,
                   "error", error_message);
}

bool IsSignedExchangeHandlingEnabled() {
  return base::FeatureList::IsEnabled(features::kSignedHTTPExchange) ||
         base::FeatureList::IsEnabled(features::kSignedHTTPExchangeOriginTrial);
}

bool ShouldHandleAsSignedHTTPExchange(
    const GURL& request_url,
    const network::ResourceResponseHead& head) {
  // Currently we don't support the signed exchange which is returned from a
  // service worker.
  // TODO(crbug/803774): Decide whether we should support it or not.
  if (head.was_fetched_via_service_worker)
    return false;
  if (!WebPackageRequestHandler::IsSupportedMimeType(head.mime_type))
    return false;
  if (base::FeatureList::IsEnabled(features::kSignedHTTPExchange))
    return true;
  if (!base::FeatureList::IsEnabled(features::kSignedHTTPExchangeOriginTrial))
    return false;
  std::unique_ptr<blink::TrialTokenValidator> validator =
      std::make_unique<blink::TrialTokenValidator>();
  return validator->RequestEnablesFeature(request_url, head.headers.get(),
                                          features::kSignedHTTPExchange.name,
                                          base::Time::Now());
}

}  // namespace signed_exchange_utils
}  // namespace content
