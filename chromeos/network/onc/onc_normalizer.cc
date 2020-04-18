// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/onc/onc_normalizer.h"

#include <string>

#include "base/logging.h"
#include "base/values.h"
#include "chromeos/network/network_event_log.h"
#include "chromeos/network/onc/onc_signature.h"
#include "chromeos/network/onc/onc_utils.h"
#include "components/onc/onc_constants.h"

namespace chromeos {
namespace onc {

Normalizer::Normalizer(bool remove_recommended_fields)
    : remove_recommended_fields_(remove_recommended_fields) {
}

Normalizer::~Normalizer() = default;

std::unique_ptr<base::DictionaryValue> Normalizer::NormalizeObject(
    const OncValueSignature* object_signature,
    const base::DictionaryValue& onc_object) {
  CHECK(object_signature != NULL);
  bool error = false;
  std::unique_ptr<base::DictionaryValue> result =
      MapObject(*object_signature, onc_object, &error);
  DCHECK(!error);
  return result;
}

std::unique_ptr<base::DictionaryValue> Normalizer::MapObject(
    const OncValueSignature& signature,
    const base::DictionaryValue& onc_object,
    bool* error) {
  std::unique_ptr<base::DictionaryValue> normalized =
      Mapper::MapObject(signature, onc_object, error);

  if (normalized.get() == NULL)
    return std::unique_ptr<base::DictionaryValue>();

  if (remove_recommended_fields_)
    normalized->RemoveWithoutPathExpansion(::onc::kRecommended, NULL);

  if (&signature == &kCertificateSignature)
    NormalizeCertificate(normalized.get());
  else if (&signature == &kEAPSignature)
    NormalizeEAP(normalized.get());
  else if (&signature == &kEthernetSignature)
    NormalizeEthernet(normalized.get());
  else if (&signature == &kIPsecSignature)
    NormalizeIPsec(normalized.get());
  else if (&signature == &kNetworkConfigurationSignature)
    NormalizeNetworkConfiguration(normalized.get());
  else if (&signature == &kOpenVPNSignature)
    NormalizeOpenVPN(normalized.get());
  else if (&signature == &kProxySettingsSignature)
    NormalizeProxySettings(normalized.get());
  else if (&signature == &kVPNSignature)
    NormalizeVPN(normalized.get());
  else if (&signature == &kWiFiSignature)
    NormalizeWiFi(normalized.get());

  return normalized;
}

namespace {

void RemoveEntryUnless(base::DictionaryValue* dict,
                       const std::string& path,
                       bool condition) {
  if (!condition && dict->FindKey(path)) {
    NET_LOG(ERROR) << "onc::Normalizer:Removing: " << path;
    dict->RemoveKey(path);
  }
}

}  // namespace

void Normalizer::NormalizeCertificate(base::DictionaryValue* cert) {
  using namespace ::onc::certificate;

  std::string type;
  cert->GetStringWithoutPathExpansion(::onc::certificate::kType, &type);
  RemoveEntryUnless(cert, kPKCS12, type == kClient);
  RemoveEntryUnless(cert, kTrustBits, type == kServer || type == kAuthority);
  RemoveEntryUnless(cert, kX509, type == kServer || type == kAuthority);
}

void Normalizer::NormalizeEthernet(base::DictionaryValue* ethernet) {
  using namespace ::onc::ethernet;

  std::string auth;
  ethernet->GetStringWithoutPathExpansion(kAuthentication, &auth);
  RemoveEntryUnless(ethernet, kEAP, auth == k8021X);
}

void Normalizer::NormalizeEAP(base::DictionaryValue* eap) {
  using namespace ::onc::eap;

  std::string clientcert_type;
  eap->GetStringWithoutPathExpansion(::onc::client_cert::kClientCertType,
                                     &clientcert_type);
  RemoveEntryUnless(eap,
                    ::onc::client_cert::kClientCertPattern,
                    clientcert_type == ::onc::client_cert::kPattern);
  RemoveEntryUnless(eap,
                    ::onc::client_cert::kClientCertRef,
                    clientcert_type == ::onc::client_cert::kRef);

  std::string outer;
  eap->GetStringWithoutPathExpansion(kOuter, &outer);
  RemoveEntryUnless(eap, kAnonymousIdentity,
                    outer == kPEAP || outer == kEAP_TTLS);
  RemoveEntryUnless(eap, kInner,
                    outer == kPEAP || outer == kEAP_TTLS || outer == kEAP_FAST);
}

void Normalizer::NormalizeIPsec(base::DictionaryValue* ipsec) {
  using namespace ::onc::ipsec;

  std::string auth_type;
  ipsec->GetStringWithoutPathExpansion(kAuthenticationType, &auth_type);
  RemoveEntryUnless(
      ipsec, ::onc::client_cert::kClientCertType, auth_type == kCert);
  RemoveEntryUnless(ipsec, kServerCARef, auth_type == kCert);
  RemoveEntryUnless(ipsec, kPSK, auth_type == kPSK);
  RemoveEntryUnless(ipsec, ::onc::vpn::kSaveCredentials, auth_type == kPSK);

  std::string clientcert_type;
  ipsec->GetStringWithoutPathExpansion(::onc::client_cert::kClientCertType,
                                       &clientcert_type);
  RemoveEntryUnless(ipsec,
                    ::onc::client_cert::kClientCertPattern,
                    clientcert_type == ::onc::client_cert::kPattern);
  RemoveEntryUnless(ipsec,
                    ::onc::client_cert::kClientCertRef,
                    clientcert_type == ::onc::client_cert::kRef);

  int ike_version = -1;
  ipsec->GetIntegerWithoutPathExpansion(kIKEVersion, &ike_version);
  RemoveEntryUnless(ipsec, kEAP, ike_version == 2);
  RemoveEntryUnless(ipsec, kGroup, ike_version == 1);
  RemoveEntryUnless(ipsec, kXAUTH, ike_version == 1);
}

void Normalizer::NormalizeNetworkConfiguration(base::DictionaryValue* network) {
  bool remove = false;
  network->GetBooleanWithoutPathExpansion(::onc::kRemove, &remove);
  if (remove) {
    network->RemoveWithoutPathExpansion(::onc::network_config::kStaticIPConfig,
                                        NULL);
    network->RemoveWithoutPathExpansion(::onc::network_config::kName, NULL);
    network->RemoveWithoutPathExpansion(::onc::network_config::kProxySettings,
                                        NULL);
    network->RemoveWithoutPathExpansion(::onc::network_config::kType, NULL);
    // Fields dependent on kType are removed afterwards, too.
  }

  std::string type;
  network->GetStringWithoutPathExpansion(::onc::network_config::kType, &type);
  RemoveEntryUnless(network,
                    ::onc::network_config::kEthernet,
                    type == ::onc::network_type::kEthernet);
  RemoveEntryUnless(
      network, ::onc::network_config::kVPN, type == ::onc::network_type::kVPN);
  RemoveEntryUnless(network,
                    ::onc::network_config::kWiFi,
                    type == ::onc::network_type::kWiFi);

  std::string ip_address_config_type, name_servers_config_type;
  network->GetStringWithoutPathExpansion(
      ::onc::network_config::kIPAddressConfigType, &ip_address_config_type);
  network->GetStringWithoutPathExpansion(
      ::onc::network_config::kNameServersConfigType, &name_servers_config_type);
  RemoveEntryUnless(
      network, ::onc::network_config::kStaticIPConfig,
      (ip_address_config_type == ::onc::network_config::kIPConfigTypeStatic) ||
          (name_servers_config_type ==
           ::onc::network_config::kIPConfigTypeStatic));
  // TODO(pneubeck): Remove fields from StaticIPConfig not specified by
  // IP[Address|Nameservers]ConfigType.
}

void Normalizer::NormalizeOpenVPN(base::DictionaryValue* openvpn) {
  std::string clientcert_type;
  openvpn->GetStringWithoutPathExpansion(::onc::client_cert::kClientCertType,
                                         &clientcert_type);
  RemoveEntryUnless(openvpn,
                    ::onc::client_cert::kClientCertPattern,
                    clientcert_type == ::onc::client_cert::kPattern);
  RemoveEntryUnless(openvpn,
                    ::onc::client_cert::kClientCertRef,
                    clientcert_type == ::onc::client_cert::kRef);

  base::Value* user_auth_type_value = openvpn->FindKeyOfType(
      ::onc::openvpn::kUserAuthenticationType, base::Value::Type::STRING);
  // If UserAuthenticationType is unspecified, do not strip Password and OTP.
  if (!user_auth_type_value)
    return;
  std::string user_auth_type = user_auth_type_value->GetString();
  RemoveEntryUnless(
      openvpn,
      ::onc::openvpn::kPassword,
      user_auth_type == ::onc::openvpn_user_auth_type::kPassword ||
          user_auth_type == ::onc::openvpn_user_auth_type::kPasswordAndOTP);
  RemoveEntryUnless(
      openvpn,
      ::onc::openvpn::kOTP,
      user_auth_type == ::onc::openvpn_user_auth_type::kOTP ||
          user_auth_type == ::onc::openvpn_user_auth_type::kPasswordAndOTP);
}

void Normalizer::NormalizeProxySettings(base::DictionaryValue* proxy) {
  using namespace ::onc::proxy;

  std::string type;
  proxy->GetStringWithoutPathExpansion(::onc::proxy::kType, &type);
  RemoveEntryUnless(proxy, kManual, type == kManual);
  RemoveEntryUnless(proxy, kExcludeDomains, type == kManual);
  RemoveEntryUnless(proxy, kPAC, type == kPAC);
}

void Normalizer::NormalizeVPN(base::DictionaryValue* vpn) {
  using namespace ::onc::vpn;

  std::string type;
  vpn->GetStringWithoutPathExpansion(::onc::vpn::kType, &type);
  RemoveEntryUnless(vpn, kOpenVPN, type == kOpenVPN);
  RemoveEntryUnless(vpn, kIPsec, type == kIPsec || type == kTypeL2TP_IPsec);
  RemoveEntryUnless(vpn, kL2TP, type == kTypeL2TP_IPsec);
  RemoveEntryUnless(vpn, kThirdPartyVpn, type == kThirdPartyVpn);
  RemoveEntryUnless(vpn, kArcVpn, type == kArcVpn);
}

void Normalizer::NormalizeWiFi(base::DictionaryValue* wifi) {
  using namespace ::onc::wifi;

  std::string security;
  wifi->GetStringWithoutPathExpansion(::onc::wifi::kSecurity, &security);
  RemoveEntryUnless(wifi, kEAP, security == kWEP_8021X || security == kWPA_EAP);
  RemoveEntryUnless(wifi, kPassphrase,
                    security == kWEP_PSK || security == kWPA_PSK);
  FillInHexSSIDField(wifi);
}

}  // namespace onc
}  // namespace chromeos
