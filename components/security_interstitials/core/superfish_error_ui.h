// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SECURITY_INTERSTITIALS_CORE_SUPERFISH_ERROR_UI_H_
#define COMPONENTS_SECURITY_INTERSTITIALS_CORE_SUPERFISH_ERROR_UI_H_

#include "components/security_interstitials/core/ssl_error_ui.h"

namespace security_interstitials {

class ControllerClient;

// A subclass of SSLErrorUI that customizes the error UI with instructions for
// uninstalling the Superfish software.
class SuperfishErrorUI : public SSLErrorUI {
 public:
  SuperfishErrorUI(
      const GURL& request_url,
      int cert_error,
      const net::SSLInfo& ssl_info,
      int display_options,  // Bitmask of SSLErrorOptionsMask values.
      const base::Time& time_triggered,
      ControllerClient* controller);
  ~SuperfishErrorUI() override {}

  void PopulateStringsForHTML(base::DictionaryValue* load_time_data) override;
  void HandleCommand(SecurityInterstitialCommand command) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SuperfishErrorUI);
};

}  // namespace security_interstitials

#endif  // COMPONENTS_SECURITY_INTERSTITIALS_CORE_SUPERFISH_ERROR_UI_H_
