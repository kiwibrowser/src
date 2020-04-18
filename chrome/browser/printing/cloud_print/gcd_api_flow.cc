// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/cloud_print/gcd_api_flow.h"

#include <memory>

#include "chrome/browser/printing/cloud_print/gcd_api_flow_impl.h"
#include "chrome/browser/printing/cloud_print/gcd_constants.h"
#include "chrome/common/cloud_print/cloud_print_constants.h"
#include "components/cloud_devices/common/cloud_devices_urls.h"

namespace cloud_print {

GCDApiFlow::Request::~Request() {
}

std::unique_ptr<GCDApiFlow> GCDApiFlow::Create(
    net::URLRequestContextGetter* request_context,
    OAuth2TokenService* token_service,
    const std::string& account_id) {
  return std::unique_ptr<GCDApiFlow>(
      new GCDApiFlowImpl(request_context, token_service, account_id));
}

GCDApiFlow::GCDApiFlow() {
}

GCDApiFlow::~GCDApiFlow() {
}

CloudPrintApiFlowRequest::CloudPrintApiFlowRequest() {
}

CloudPrintApiFlowRequest::~CloudPrintApiFlowRequest() {
}

std::string CloudPrintApiFlowRequest::GetOAuthScope() {
  return cloud_devices::kCloudPrintAuthScope;
}

std::vector<std::string> CloudPrintApiFlowRequest::GetExtraRequestHeaders() {
  return std::vector<std::string>(1, cloud_print::kChromeCloudPrintProxyHeader);
}

}  // namespace cloud_print
