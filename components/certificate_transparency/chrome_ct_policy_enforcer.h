// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CERTIFICATE_TRANSPARENCY_CHROME_CT_POLICY_ENFORCER_H_
#define COMPONENTS_CERTIFICATE_TRANSPARENCY_CHROME_CT_POLICY_ENFORCER_H_

#include "net/cert/ct_policy_enforcer.h"

namespace certificate_transparency {

// A CTPolicyEnforcer that enforces the "Certificate Transparency in Chrome"
// policies detailed at
// https://github.com/chromium/ct-policy/blob/master/ct_policy.md
//
// This should only be used when there is a reliable, rapid update mechanism
// for the set of known, qualified logs - either through a reliable binary
// updating mechanism or through out-of-band delivery. See
// //net/docs/certificate-transparency.md for more details.
class ChromeCTPolicyEnforcer : public net::CTPolicyEnforcer {
 public:
  ChromeCTPolicyEnforcer() = default;
  ~ChromeCTPolicyEnforcer() override = default;

  net::ct::CTPolicyCompliance CheckCompliance(
      net::X509Certificate* cert,
      const net::ct::SCTList& verified_scts,
      const net::NetLogWithSource& net_log) override;
};

}  // namespace certificate_transparency

#endif  // COMPONENTS_CERTIFICATE_TRANSPARENCY_CHROME_CT_POLICY_ENFORCER_H_
