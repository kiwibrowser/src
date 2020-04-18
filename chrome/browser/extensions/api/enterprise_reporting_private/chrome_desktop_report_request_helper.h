// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_ENTERPRISE_REPORTING_PRIVATE_CHROME_DESKTOP_REPORT_REQUEST_HELPER_H_
#define CHROME_BROWSER_EXTENSIONS_API_ENTERPRISE_REPORTING_PRIVATE_CHROME_DESKTOP_REPORT_REQUEST_HELPER_H_

#include <memory>

#include "components/policy/proto/device_management_backend.pb.h"

class Profile;

namespace base {
class DictionaryValue;
}  // namespace base

namespace extensions {

// Transfer the input from Json file to protobuf. Return nullptr if the input
// is not valid.
std::unique_ptr<enterprise_management::ChromeDesktopReportRequest>
GenerateChromeDesktopReportRequest(const base::DictionaryValue& report,
                                   Profile* profile);

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_ENTERPRISE_REPORTING_PRIVATE_CHROME_DESKTOP_REPORT_REQUEST_HELPER_H_
