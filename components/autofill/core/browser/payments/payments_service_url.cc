// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/payments/payments_service_url.h"

#include <string>

#include "base/command_line.h"
#include "base/format_macros.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "google_apis/gaia/gaia_urls.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace autofill {
namespace {

const char kProdPaymentsServiceUrl[] = "https://payments.google.com/";

const char kSandboxPaymentsSecureServiceUrl[] =
    "https://payments.sandbox.google.com/";

}  // namespace

namespace payments {

bool IsPaymentsProductionEnabled() {
  // If the command line flag exists, it takes precedence.
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  std::string sandbox_enabled(
      command_line->GetSwitchValueASCII(switches::kWalletServiceUseSandbox));
  return sandbox_enabled.empty() || sandbox_enabled != "1";
}

GURL GetBaseSecureUrl() {
  return GURL(IsPaymentsProductionEnabled() ? kProdPaymentsServiceUrl
                                            : kSandboxPaymentsSecureServiceUrl);
}

GURL GetManageInstrumentsUrl(size_t user_index) {
  std::string path =
      base::StringPrintf("u/%" PRIuS "#paymentMethods", user_index);
  return GetBaseSecureUrl().Resolve(path);
}

GURL GetManageAddressesUrl(size_t user_index) {
  // Billing addresses are now managed as a part of the payment instrument.
  return GetManageInstrumentsUrl(user_index);
}

}  // namespace payments
}  // namespace autofill
