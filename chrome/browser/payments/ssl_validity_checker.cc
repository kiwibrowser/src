// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/payments/ssl_validity_checker.h"

#include "base/logging.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "components/security_state/core/security_state.h"

namespace payments {

// static
bool SslValidityChecker::IsSslCertificateValid(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  SecurityStateTabHelper::CreateForWebContents(web_contents);
  SecurityStateTabHelper* helper =
      SecurityStateTabHelper::FromWebContents(web_contents);
  DCHECK(helper);
  security_state::SecurityInfo security_info;
  helper->GetSecurityInfo(&security_info);
  return security_info.security_level == security_state::EV_SECURE ||
         security_info.security_level == security_state::SECURE ||
         security_info.security_level ==
             security_state::SECURE_WITH_POLICY_INSTALLED_CERT;
}

}  // namespace payments
