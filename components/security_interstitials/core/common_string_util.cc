// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/security_interstitials/core/common_string_util.h"

#include "base/i18n/rtl.h"
#include "base/i18n/time_formatting.h"
#include "base/strings/strcat.h"
#include "components/strings/grit/components_strings.h"
#include "components/url_formatter/url_formatter.h"
#include "net/base/net_errors.h"
#include "ui/base/l10n/l10n_util.h"

namespace security_interstitials {

namespace common_string_util {

base::string16 GetFormattedHostName(const GURL& gurl) {
  base::string16 host = url_formatter::IDNToUnicode(gurl.host());
  if (base::i18n::IsRTL())
    base::i18n::WrapStringWithLTRFormatting(&host);
  return host;
}

void PopulateSSLLayoutStrings(int cert_error,
                              base::DictionaryValue* load_time_data) {
  load_time_data->SetString("type", "SSL");
  load_time_data->SetString("errorCode", net::ErrorToString(cert_error));
  load_time_data->SetString(
      "openDetails", l10n_util::GetStringUTF16(IDS_SSL_OPEN_DETAILS_BUTTON));
  load_time_data->SetString(
      "closeDetails", l10n_util::GetStringUTF16(IDS_SSL_CLOSE_DETAILS_BUTTON));
  // Not used by most interstitials; can be overridden by individual
  // interstitials as needed.
  load_time_data->SetString("recurrentErrorParagraph", "");
  load_time_data->SetBoolean("show_recurrent_error_paragraph", false);
}

void PopulateSSLDebuggingStrings(const net::SSLInfo ssl_info,
                                 const base::Time time_triggered,
                                 base::DictionaryValue* load_time_data) {
  load_time_data->SetString("subject",
                            ssl_info.cert->subject().GetDisplayName());
  load_time_data->SetString("issuer", ssl_info.cert->issuer().GetDisplayName());
  load_time_data->SetString(
      "expirationDate",
      base::TimeFormatShortDate(ssl_info.cert->valid_expiry()));
  load_time_data->SetString("currentDate",
                            base::TimeFormatShortDate(time_triggered));
  std::vector<std::string> encoded_chain;
  ssl_info.cert->GetPEMEncodedChain(&encoded_chain);
  load_time_data->SetString("pem", base::StrCat(encoded_chain));
}

}  // namespace common_string_util

}  // namespace security_interstitials
