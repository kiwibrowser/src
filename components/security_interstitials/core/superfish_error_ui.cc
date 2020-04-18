// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/security_interstitials/core/superfish_error_ui.h"

#include "components/security_interstitials/core/common_string_util.h"
#include "components/security_interstitials/core/controller_client.h"
#include "components/security_interstitials/core/metrics_helper.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace security_interstitials {

namespace {

// URL for Superfish-specific help page.
const char kHelpURL[] = "https://support.google.com/chrome/?p=superfish";

}  // namespace

SuperfishErrorUI::SuperfishErrorUI(
    const GURL& request_url,
    int cert_error,
    const net::SSLInfo& ssl_info,
    int display_options,  // Bitmask of SSLErrorOptionsMask values.
    const base::Time& time_triggered,
    ControllerClient* controller)
    : SSLErrorUI(request_url,
                 cert_error,
                 ssl_info,
                 display_options,
                 time_triggered,
                 GURL(),
                 controller) {}

void SuperfishErrorUI::PopulateStringsForHTML(
    base::DictionaryValue* load_time_data) {
  common_string_util::PopulateSSLDebuggingStrings(ssl_info(), time_triggered(),
                                                  load_time_data);

  load_time_data->SetString("type", "SSL");
  load_time_data->SetString("errorCode", net::ErrorToString(cert_error()));
  load_time_data->SetBoolean("overridable", false);
  load_time_data->SetBoolean("bad_clock", false);
  load_time_data->SetBoolean("hide_primary_button", true);
  load_time_data->SetString("tabTitle",
                            l10n_util::GetStringUTF16(IDS_SSL_V2_TITLE));
  load_time_data->SetString(
      "heading", l10n_util::GetStringUTF16(IDS_SSL_SUPERFISH_HEADING));
  load_time_data->SetString(
      "primaryParagraph",
      l10n_util::GetStringUTF16(IDS_SSL_SUPERFISH_PRIMARY_PARAGRAPH));

  // Fill in empty values for normal SSL error strings that aren't used on this
  // interstitial.
  load_time_data->SetString("explanationParagraph", std::string());
  load_time_data->SetString("primaryButtonText", std::string());
  load_time_data->SetString("finalParagraph", std::string());
  load_time_data->SetString("openDetails", base::string16());
  load_time_data->SetString("closeDetails", base::string16());
  load_time_data->SetString("recurrentErrorParagraph", base::string16());
  load_time_data->SetBoolean("show_recurrent_error_paragraph", false);
}

void SuperfishErrorUI::HandleCommand(SecurityInterstitialCommand command) {
  // Override the Help Center link to point to a Superfish-specific page.
  if (command == CMD_OPEN_HELP_CENTER) {
    controller()->metrics_helper()->RecordUserInteraction(
        security_interstitials::MetricsHelper::SHOW_LEARN_MORE);
    controller()->OpenUrlInNewForegroundTab(GURL(kHelpURL));
    return;
  }
  SSLErrorUI::HandleCommand(command);
}

}  // namespace security_interstitials
