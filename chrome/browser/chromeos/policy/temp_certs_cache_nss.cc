// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/temp_certs_cache_nss.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/policy/device_network_configuration_updater.h"
#include "chromeos/network/onc/onc_utils.h"

namespace policy {

TempCertsCacheNSS::TempCertsCacheNSS(
    const std::vector<std::string>& x509_certs) {
  for (const auto& x509_cert : x509_certs) {
    net::ScopedCERTCertificate temp_cert =
        chromeos::onc::DecodePEMCertificate(x509_cert);
    if (!temp_cert) {
      LOG(ERROR) << "Unable to create untrusted authority certificate from PEM "
                    "encoding";
      continue;
    }

    temp_certs_.push_back(std::move(temp_cert));
  }
}

TempCertsCacheNSS::~TempCertsCacheNSS() {}

// static
std::vector<std::string>
TempCertsCacheNSS::GetUntrustedAuthoritiesFromDeviceOncPolicy() {
  return g_browser_process->platform_part()
      ->browser_policy_connector_chromeos()
      ->GetDeviceNetworkConfigurationUpdater()
      ->GetAuthorityCertificates();
}

}  // namespace policy
