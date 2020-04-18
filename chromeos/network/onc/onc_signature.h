// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_ONC_ONC_SIGNATURE_H_
#define CHROMEOS_NETWORK_ONC_ONC_SIGNATURE_H_

#include <string>

#include "base/values.h"
#include "chromeos/chromeos_export.h"

namespace chromeos {
namespace onc {

struct OncValueSignature;

struct OncFieldSignature {
  const char* onc_field_name;
  const OncValueSignature* value_signature;
};

struct CHROMEOS_EXPORT OncValueSignature {
  base::Value::Type onc_type;
  const OncFieldSignature* fields;
  const OncValueSignature* onc_array_entry_signature;
  const OncValueSignature* base_signature;
};

CHROMEOS_EXPORT const OncFieldSignature* GetFieldSignature(
    const OncValueSignature& signature,
    const std::string& onc_field_name);

CHROMEOS_EXPORT bool FieldIsCredential(const OncValueSignature& signature,
                                       const std::string& onc_field_name);

CHROMEOS_EXPORT extern const OncValueSignature kRecommendedSignature;
CHROMEOS_EXPORT extern const OncValueSignature kEAPSignature;
CHROMEOS_EXPORT extern const OncValueSignature kIssuerSubjectPatternSignature;
CHROMEOS_EXPORT extern const OncValueSignature kCertificatePatternSignature;
CHROMEOS_EXPORT extern const OncValueSignature kIPsecSignature;
CHROMEOS_EXPORT extern const OncValueSignature kL2TPSignature;
CHROMEOS_EXPORT extern const OncValueSignature kXAUTHSignature;
CHROMEOS_EXPORT extern const OncValueSignature kOpenVPNSignature;
CHROMEOS_EXPORT extern const OncValueSignature kThirdPartyVPNSignature;
CHROMEOS_EXPORT extern const OncValueSignature kARCVPNSignature;
CHROMEOS_EXPORT extern const OncValueSignature kVerifyX509Signature;
CHROMEOS_EXPORT extern const OncValueSignature kVPNSignature;
CHROMEOS_EXPORT extern const OncValueSignature kEthernetSignature;
CHROMEOS_EXPORT extern const OncValueSignature kTetherSignature;
CHROMEOS_EXPORT extern const OncValueSignature kTetherWithStateSignature;
CHROMEOS_EXPORT extern const OncValueSignature kIPConfigSignature;
CHROMEOS_EXPORT extern const OncValueSignature kSavedIPConfigSignature;
CHROMEOS_EXPORT extern const OncValueSignature kStaticIPConfigSignature;
CHROMEOS_EXPORT extern const OncValueSignature kProxyLocationSignature;
CHROMEOS_EXPORT extern const OncValueSignature kProxyManualSignature;
CHROMEOS_EXPORT extern const OncValueSignature kProxySettingsSignature;
CHROMEOS_EXPORT extern const OncValueSignature kWiFiSignature;
CHROMEOS_EXPORT extern const OncValueSignature kWiMAXSignature;
CHROMEOS_EXPORT extern const OncValueSignature kCertificateSignature;
CHROMEOS_EXPORT extern const OncValueSignature kNetworkConfigurationSignature;
CHROMEOS_EXPORT extern const OncValueSignature
    kGlobalNetworkConfigurationSignature;
CHROMEOS_EXPORT extern const OncValueSignature kCertificateListSignature;
CHROMEOS_EXPORT extern const OncValueSignature
    kNetworkConfigurationListSignature;
CHROMEOS_EXPORT extern const OncValueSignature kToplevelConfigurationSignature;

// Derived "ONC with State" signatures.
CHROMEOS_EXPORT extern const OncValueSignature kNetworkWithStateSignature;
CHROMEOS_EXPORT extern const OncValueSignature kWiFiWithStateSignature;
CHROMEOS_EXPORT extern const OncValueSignature kWiMAXWithStateSignature;
CHROMEOS_EXPORT extern const OncValueSignature kCellularSignature;
CHROMEOS_EXPORT extern const OncValueSignature kCellularWithStateSignature;
CHROMEOS_EXPORT extern const OncValueSignature kCellularPaymentPortalSignature;
CHROMEOS_EXPORT extern const OncValueSignature kCellularProviderSignature;
CHROMEOS_EXPORT extern const OncValueSignature kCellularApnSignature;
CHROMEOS_EXPORT extern const OncValueSignature kCellularFoundNetworkSignature;
CHROMEOS_EXPORT extern const OncValueSignature kSIMLockStatusSignature;

}  // namespace onc
}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_ONC_ONC_SIGNATURE_H_
