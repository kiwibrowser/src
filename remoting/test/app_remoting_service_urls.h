// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_APP_REMOTING_SERVICE_URLS_H_
#define REMOTING_TEST_APP_REMOTING_SERVICE_URLS_H_

#include <string>

namespace remoting {
namespace test {

// Specifies the service API to call for app remoting host information.
// Note: When adding new environments, add them before kUnknownEnvironment as
//       the last entry is used for bounds checking.
enum ServiceEnvironment {
  kDeveloperEnvironment,
  kUnknownEnvironment
};

// Used to determine if the service_environment is one of the supported values.
bool IsSupportedServiceEnvironment(ServiceEnvironment service_environment);

// Generates and returns a URL for the specified application and environment to
// request remote host details.
std::string GetRunApplicationUrl(const std::string& extension_id,
                                 ServiceEnvironment service_environment);

// Generates and returns a URL for the specified application and environment to
// report an issue.
std::string GetReportIssueUrl(const std::string& extension_id,
                              const std::string& host_id,
                              ServiceEnvironment service_environment);

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_APP_REMOTING_SERVICE_URLS_H_
