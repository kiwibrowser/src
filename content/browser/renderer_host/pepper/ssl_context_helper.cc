// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/ssl_context_helper.h"

#include "net/cert/cert_verifier.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/http/transport_security_state.h"

namespace content {

SSLContextHelper::SSLContextHelper() {}

SSLContextHelper::~SSLContextHelper() {}

net::CertVerifier* SSLContextHelper::GetCertVerifier() {
  if (!cert_verifier_)
    cert_verifier_ = net::CertVerifier::CreateDefault();
  return cert_verifier_.get();
}

net::TransportSecurityState* SSLContextHelper::GetTransportSecurityState() {
  if (!transport_security_state_)
    transport_security_state_.reset(new net::TransportSecurityState());
  return transport_security_state_.get();
}

net::CTVerifier* SSLContextHelper::GetCertTransparencyVerifier() {
  if (!cert_transparency_verifier_)
    cert_transparency_verifier_.reset(new net::MultiLogCTVerifier());
  return cert_transparency_verifier_.get();
}

net::CTPolicyEnforcer* SSLContextHelper::GetCTPolicyEnforcer() {
  if (!ct_policy_enforcer_)
    ct_policy_enforcer_.reset(new net::DefaultCTPolicyEnforcer());
  return ct_policy_enforcer_.get();
}

}  // namespace content
