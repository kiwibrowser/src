// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAYMENTS_SSL_VALIDITY_CHECKER_H_
#define CHROME_BROWSER_PAYMENTS_SSL_VALIDITY_CHECKER_H_

#include "base/macros.h"

namespace content {
class WebContents;
}

namespace payments {

class SslValidityChecker {
 public:
  // Returns true for |web_contents| with a valid SSL certificate. Only
  // EV_SECURE, SECURE, and SECURE_WITH_POLICY_INSTALLED_CERT are considered
  // valid for web payments.
  static bool IsSslCertificateValid(content::WebContents* web_contents);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SslValidityChecker);
};

}  // namespace payments

#endif  // CHROME_BROWSER_PAYMENTS_SSL_VALIDITY_CHECKER_H_
