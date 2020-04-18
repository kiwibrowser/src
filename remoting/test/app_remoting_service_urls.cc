// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/app_remoting_service_urls.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"

namespace {
// The placeholder is the environment endpoint qualifier.  No trailing slash
// is added as it will be appended as needed later.
const char kAppRemotingTestEndpointBase[] =
    "https://www-googleapis-test.sandbox.google.com/appremoting/%s";
const char kAppRemotingDevEndpointQualifier[] = "v1beta1_dev";

// Placeholder value is for the Application ID.
const char kRunApplicationApi[] = "applications/%s/run";

// First placeholder value is for the Application ID.  Second placeholder is for
// the Host ID to report the issue for.
const char kReportIssueApi[] = "applications/%s/hosts/%s/reportIssue";
}  // namespace

namespace remoting {
namespace test {

bool IsSupportedServiceEnvironment(ServiceEnvironment service_environment) {
  return (service_environment >= 0 &&
          service_environment < kUnknownEnvironment);
}

std::string GetBaseUrl(ServiceEnvironment service_environment) {
  std::string base_service_url;

  if (service_environment == kDeveloperEnvironment) {
    base_service_url = base::StringPrintf(kAppRemotingTestEndpointBase,
                                          kAppRemotingDevEndpointQualifier);
  }

  return base_service_url;
}

std::string GetRunApplicationUrl(const std::string& extension_id,
                                 ServiceEnvironment service_environment) {
  std::string service_url;
  if (!IsSupportedServiceEnvironment(service_environment)) {
    return service_url;
  }

  service_url = GetBaseUrl(service_environment);
  if (!service_url.empty()) {
    std::string api_string =
        base::StringPrintf(kRunApplicationApi, extension_id.c_str());
    service_url =
        base::StringPrintf("%s/%s", service_url.c_str(), api_string.c_str());
  }

  return service_url;
}

std::string GetReportIssueUrl(const std::string& extension_id,
                              const std::string& host_id,
                              ServiceEnvironment service_environment) {
  std::string service_url;
  if (!IsSupportedServiceEnvironment(service_environment)) {
    return service_url;
  }

  service_url = GetBaseUrl(service_environment);
  if (!service_url.empty()) {
    std::string api_string = base::StringPrintf(
        kReportIssueApi, extension_id.c_str(), host_id.c_str());
    service_url =
        base::StringPrintf("%s/%s", service_url.c_str(), api_string.c_str());
  }

  return service_url;
}

}  // namespace test
}  // namespace remoting
