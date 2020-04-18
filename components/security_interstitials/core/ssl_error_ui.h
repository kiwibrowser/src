// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SECURITY_INTERSTITIALS_CORE_SSL_ERROR_UI_H_
#define COMPONENTS_SECURITY_INTERSTITIALS_CORE_SSL_ERROR_UI_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/security_interstitials/core/controller_client.h"
#include "net/ssl/ssl_info.h"
#include "url/gurl.h"

namespace security_interstitials {

class ControllerClient;

// This class displays UI for SSL errors that block page loads. This class is
// purely about visual display; it does not do any error-handling logic to
// determine what type of error should be displayed when.
class SSLErrorUI {
 public:
  enum SSLErrorOptionsMask {
    // Indicates that the error UI should support dismissing the error and
    // loading the page. By default, the errors cannot be overridden via the UI.
    SOFT_OVERRIDE_ENABLED = 1 << 0,
    // Indicates that the user should NOT be allowed to use a "secret code" to
    // dismiss the error and load the page, even if the UI does not support it.
    // By default, an error can be overridden via the "secret code."
    HARD_OVERRIDE_DISABLED = 1 << 1,
    // Indicates that the site the user is trying to connect to has requested
    // strict enforcement of certificate validation (e.g. with HTTP
    // Strict-Transport-Security). By default, the error assumes strict
    // enforcement was not requested.
    STRICT_ENFORCEMENT = 1 << 2,
    // Indicates that a user decision had been previously made but the
    // decision has expired.
    EXPIRED_BUT_PREVIOUSLY_ALLOWED = 1 << 3,
  };

  SSLErrorUI(const GURL& request_url,
             int cert_error,
             const net::SSLInfo& ssl_info,
             int display_options,  // Bitmask of SSLErrorOptionsMask values.
             const base::Time& time_triggered,
             const GURL& support_url,
             ControllerClient* controller);
  virtual ~SSLErrorUI();

  virtual void PopulateStringsForHTML(base::DictionaryValue* load_time_data);
  virtual void HandleCommand(SecurityInterstitialCommand command);

 protected:
  const net::SSLInfo& ssl_info() const;
  const base::Time& time_triggered() const;
  ControllerClient* controller() const;
  int cert_error() const;

 private:
  void PopulateOverridableStrings(base::DictionaryValue* load_time_data);
  void PopulateNonOverridableStrings(base::DictionaryValue* load_time_data);

  const GURL request_url_;
  const int cert_error_;
  const net::SSLInfo ssl_info_;
  const base::Time time_triggered_;
  const GURL support_url_;

  // Set by the |display_options|.
  const bool requested_strict_enforcement_;
  const bool soft_override_enabled_;  // UI provides a button to dismiss error.
  const bool hard_override_enabled_;  // Dismissing allowed without button.

  ControllerClient* controller_;
  bool user_made_decision_;  // Whether the user made a choice in the UI.

  DISALLOW_COPY_AND_ASSIGN(SSLErrorUI);
};

}  // security_interstitials

#endif  // COMPONENTS_SECURITY_INTERSTITIALS_CORE_SSL_ERROR_UI_H_
