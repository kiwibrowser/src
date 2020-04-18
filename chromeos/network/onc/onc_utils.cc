// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/onc/onc_utils.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "chromeos/network/managed_network_configuration_handler.h"
#include "chromeos/network/network_configuration_handler.h"
#include "chromeos/network/network_event_log.h"
#include "chromeos/network/network_profile.h"
#include "chromeos/network/network_profile_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_ui_data.h"
#include "chromeos/network/onc/onc_mapper.h"
#include "chromeos/network/onc/onc_normalizer.h"
#include "chromeos/network/onc/onc_signature.h"
#include "chromeos/network/onc/onc_translator.h"
#include "chromeos/network/onc/onc_utils.h"
#include "chromeos/network/onc/onc_validator.h"
#include "chromeos/network/tether_constants.h"
#include "components/account_id/account_id.h"
#include "components/device_event_log/device_event_log.h"
#include "components/onc/onc_constants.h"
#include "components/onc/onc_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/proxy_config_dictionary.h"
#include "components/url_formatter/url_fixer.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "crypto/encryptor.h"
#include "crypto/hmac.h"
#include "crypto/symmetric_key.h"
#include "net/base/host_port_pair.h"
#include "net/base/proxy_server.h"
#include "net/cert/pem_tokenizer.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util_nss.h"
#include "net/proxy_resolution/proxy_bypass_rules.h"
#include "net/proxy_resolution/proxy_config.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "url/gurl.h"
#include "url/url_constants.h"

using namespace ::onc;

namespace chromeos {
namespace onc {

namespace {

// Error messages that can be reported when decrypting encrypted ONC.
constexpr char kUnableToDecrypt[] = "Unable to decrypt encrypted ONC";
constexpr char kUnableToDecode[] = "Unable to decode encrypted ONC";

// Scheme strings for supported |net::ProxyServer::SCHEME_*| enum values.
constexpr char kDirectScheme[] = "direct";
constexpr char kQuicScheme[] = "quic";
constexpr char kSocksScheme[] = "socks";
constexpr char kSocks4Scheme[] = "socks4";
constexpr char kSocks5Scheme[] = "socks5";

// Runs |variable_expander.ExpandString| on the field |fieldname| in
// |onc_object|.
void ExpandField(const std::string& fieldname,
                 const VariableExpander& variable_expander,
                 base::DictionaryValue* onc_object) {
  std::string field_value;
  if (!onc_object->GetStringWithoutPathExpansion(fieldname, &field_value))
    return;

  variable_expander.ExpandString(&field_value);

  onc_object->SetKey(fieldname, base::Value(field_value));
}

// A |Mapper| for masking sensitive fields (e.g. credentials such as
// passphrases) in ONC.
class OncMaskValues : public Mapper {
 public:
  static std::unique_ptr<base::DictionaryValue> Mask(
      const OncValueSignature& signature,
      const base::DictionaryValue& onc_object,
      const std::string& mask) {
    OncMaskValues masker(mask);
    bool unused_error;
    return masker.MapObject(signature, onc_object, &unused_error);
  }

 protected:
  explicit OncMaskValues(const std::string& mask)
      : mask_(mask) {
  }

  std::unique_ptr<base::Value> MapField(
      const std::string& field_name,
      const OncValueSignature& object_signature,
      const base::Value& onc_value,
      bool* found_unknown_field,
      bool* error) override {
    if (FieldIsCredential(object_signature, field_name)) {
      // If it's the password field and the substitution string is used, don't
      // mask it.
      if (&object_signature == &kEAPSignature && field_name == eap::kPassword &&
          onc_value.GetString() == substitutes::kPasswordPlaceholderVerbatim) {
        return Mapper::MapField(field_name, object_signature, onc_value,
                                found_unknown_field, error);
      }
      return std::unique_ptr<base::Value>(new base::Value(mask_));
    } else {
      return Mapper::MapField(field_name, object_signature, onc_value,
                              found_unknown_field, error);
    }
  }

  // Mask to insert in place of the sensitive values.
  std::string mask_;
};

// Returns a map GUID->PEM of all server and authority certificates defined in
// the Certificates section of ONC, which is passed in as |certificates|.
CertPEMsByGUIDMap GetServerAndCACertsByGUID(
    const base::ListValue& certificates) {
  CertPEMsByGUIDMap certs_by_guid;
  for (const auto& entry : certificates) {
    const base::DictionaryValue* cert = nullptr;
    bool entry_is_dictionary = entry.GetAsDictionary(&cert);
    DCHECK(entry_is_dictionary);

    std::string guid;
    cert->GetStringWithoutPathExpansion(certificate::kGUID, &guid);
    std::string cert_type;
    cert->GetStringWithoutPathExpansion(certificate::kType, &cert_type);
    if (cert_type != certificate::kServer &&
        cert_type != certificate::kAuthority) {
      continue;
    }
    std::string x509_data;
    cert->GetStringWithoutPathExpansion(certificate::kX509, &x509_data);

    std::string der = DecodePEM(x509_data);
    std::string pem;
    if (der.empty() || !net::X509Certificate::GetPEMEncodedFromDER(der, &pem)) {
      LOG(ERROR) << "Certificate with GUID " << guid
                 << " is not in PEM encoding.";
      continue;
    }
    certs_by_guid[guid] = pem;
  }

  return certs_by_guid;
}

// Fills HexSSID fields in all entries in the |network_configs| list.
void FillInHexSSIDFieldsInNetworks(base::ListValue* network_configs) {
  for (auto& entry : *network_configs) {
    base::DictionaryValue* network = nullptr;
    entry.GetAsDictionary(&network);
    DCHECK(network);
    FillInHexSSIDFieldsInOncObject(kNetworkConfigurationSignature, network);
  }
}

// Given a GUID->PEM certificate mapping |certs_by_guid|, looks up the PEM
// encoded certificate referenced by |guid_ref|. If a match is found, sets
// |*pem_encoded| to the PEM encoded certificate and returns true. Otherwise,
// returns false.
bool GUIDRefToPEMEncoding(const CertPEMsByGUIDMap& certs_by_guid,
                          const std::string& guid_ref,
                          std::string* pem_encoded) {
  CertPEMsByGUIDMap::const_iterator it = certs_by_guid.find(guid_ref);
  if (it == certs_by_guid.end()) {
    LOG(ERROR) << "Couldn't resolve certificate reference " << guid_ref;
    return false;
  }
  *pem_encoded = it->second;
  if (pem_encoded->empty()) {
    LOG(ERROR) << "Couldn't PEM-encode certificate with GUID " << guid_ref;
    return false;
  }
  return true;
}

// Given a GUID-> PM certificate mapping |certs_by_guid|, attempts to resolve
// the certificate referenced by the |key_guid_ref| field in |onc_object|.
// * If |onc_object| has no |key_guid_ref| field, returns true.
// * If no matching certificate is found in |certs_by_guid|, returns false.
// * If a matching certificate is found, removes the |key_guid_ref| field,
//   fills the |key_pem| field in |onc_object| and returns true.
bool ResolveSingleCertRef(const CertPEMsByGUIDMap& certs_by_guid,
                          const std::string& key_guid_ref,
                          const std::string& key_pem,
                          base::DictionaryValue* onc_object) {
  std::string guid_ref;
  if (!onc_object->GetStringWithoutPathExpansion(key_guid_ref, &guid_ref))
    return true;

  std::string pem_encoded;
  if (!GUIDRefToPEMEncoding(certs_by_guid, guid_ref, &pem_encoded))
    return false;

  onc_object->RemoveWithoutPathExpansion(key_guid_ref, nullptr);
  onc_object->SetKey(key_pem, base::Value(pem_encoded));
  return true;
}

// Given a GUID-> PM certificate mapping |certs_by_guid|, attempts to resolve
// the certificates referenced by the list-of-strings field |key_guid_ref_list|
// in |onc_object|.
// * If |key_guid_ref_list| does not exist in |onc_object|, returns true.
// * If any element |key_guid_ref_list| can not be found in |certs_by_guid|,
//   aborts processing and returns false. |onc_object| is unchanged in this
//   case.
// * Otherwise, sets |key_pem_list| to be a list-of-strings field in
//   |onc_object|, containing all PEM encoded resolved certificates in order and
//   returns true.
bool ResolveCertRefList(const CertPEMsByGUIDMap& certs_by_guid,
                        const std::string& key_guid_ref_list,
                        const std::string& key_pem_list,
                        base::DictionaryValue* onc_object) {
  const base::ListValue* guid_ref_list = nullptr;
  if (!onc_object->GetListWithoutPathExpansion(key_guid_ref_list,
                                               &guid_ref_list)) {
    return true;
  }

  std::unique_ptr<base::ListValue> pem_list(new base::ListValue);
  for (const auto& entry : *guid_ref_list) {
    std::string guid_ref;
    bool entry_is_string = entry.GetAsString(&guid_ref);
    DCHECK(entry_is_string);

    std::string pem_encoded;
    if (!GUIDRefToPEMEncoding(certs_by_guid, guid_ref, &pem_encoded))
      return false;

    pem_list->AppendString(pem_encoded);
  }

  onc_object->RemoveWithoutPathExpansion(key_guid_ref_list, nullptr);
  onc_object->SetWithoutPathExpansion(key_pem_list, std::move(pem_list));
  return true;
}

// Same as |ResolveSingleCertRef|, but the output |key_pem_list| will be set to
// a list with exactly one value when resolution succeeds.
bool ResolveSingleCertRefToList(const CertPEMsByGUIDMap& certs_by_guid,
                                const std::string& key_guid_ref,
                                const std::string& key_pem_list,
                                base::DictionaryValue* onc_object) {
  std::string guid_ref;
  if (!onc_object->GetStringWithoutPathExpansion(key_guid_ref, &guid_ref))
    return true;

  std::string pem_encoded;
  if (!GUIDRefToPEMEncoding(certs_by_guid, guid_ref, &pem_encoded))
    return false;

  std::unique_ptr<base::ListValue> pem_list(new base::ListValue);
  pem_list->AppendString(pem_encoded);
  onc_object->RemoveWithoutPathExpansion(key_guid_ref, nullptr);
  onc_object->SetWithoutPathExpansion(key_pem_list, std::move(pem_list));
  return true;
}

// Resolves the reference list at |key_guid_refs| if present and otherwise the
// single reference at |key_guid_ref|. Returns whether the respective resolving
// was successful.
bool ResolveCertRefsOrRefToList(const CertPEMsByGUIDMap& certs_by_guid,
                                const std::string& key_guid_refs,
                                const std::string& key_guid_ref,
                                const std::string& key_pem_list,
                                base::DictionaryValue* onc_object) {
  if (onc_object->HasKey(key_guid_refs)) {
    if (onc_object->HasKey(key_guid_ref)) {
      LOG(ERROR) << "Found both " << key_guid_refs << " and " << key_guid_ref
                 << ". Ignoring and removing the latter.";
      onc_object->RemoveWithoutPathExpansion(key_guid_ref, nullptr);
    }
    return ResolveCertRefList(
        certs_by_guid, key_guid_refs, key_pem_list, onc_object);
  }

  // Only resolve |key_guid_ref| if |key_guid_refs| isn't present.
  return ResolveSingleCertRefToList(
      certs_by_guid, key_guid_ref, key_pem_list, onc_object);
}

// Resolve known server and authority certiifcate reference fields in
// |onc_object|.
bool ResolveServerCertRefsInObject(const CertPEMsByGUIDMap& certs_by_guid,
                                   const OncValueSignature& signature,
                                   base::DictionaryValue* onc_object) {
  if (&signature == &kCertificatePatternSignature) {
    if (!ResolveCertRefList(certs_by_guid,
                            client_cert::kIssuerCARef,
                            client_cert::kIssuerCAPEMs,
                            onc_object)) {
      return false;
    }
  } else if (&signature == &kEAPSignature) {
    if (!ResolveCertRefsOrRefToList(certs_by_guid,
                                    eap::kServerCARefs,
                                    eap::kServerCARef,
                                    eap::kServerCAPEMs,
                                    onc_object)) {
      return false;
    }
  } else if (&signature == &kIPsecSignature) {
    if (!ResolveCertRefsOrRefToList(certs_by_guid,
                                    ipsec::kServerCARefs,
                                    ipsec::kServerCARef,
                                    ipsec::kServerCAPEMs,
                                    onc_object)) {
      return false;
    }
  } else if (&signature == &kIPsecSignature ||
             &signature == &kOpenVPNSignature) {
    if (!ResolveSingleCertRef(certs_by_guid,
                              openvpn::kServerCertRef,
                              openvpn::kServerCertPEM,
                              onc_object) ||
        !ResolveCertRefsOrRefToList(certs_by_guid,
                                    openvpn::kServerCARefs,
                                    openvpn::kServerCARef,
                                    openvpn::kServerCAPEMs,
                                    onc_object)) {
      return false;
    }
  }

  // Recurse into nested objects.
  for (base::DictionaryValue::Iterator it(*onc_object); !it.IsAtEnd();
       it.Advance()) {
    base::DictionaryValue* inner_object = nullptr;
    if (!onc_object->GetDictionaryWithoutPathExpansion(it.key(), &inner_object))
      continue;

    const OncFieldSignature* field_signature =
        GetFieldSignature(signature, it.key());
    if (!field_signature)
      continue;

    if (!ResolveServerCertRefsInObject(certs_by_guid,
                                       *field_signature->value_signature,
                                       inner_object)) {
      return false;
    }
  }
  return true;
}

net::ProxyServer ConvertOncProxyLocationToHostPort(
    net::ProxyServer::Scheme default_proxy_scheme,
    const base::DictionaryValue& onc_proxy_location) {
  std::string host;
  onc_proxy_location.GetStringWithoutPathExpansion(::onc::proxy::kHost, &host);
  // Parse |host| according to the format [<scheme>"://"]<server>[":"<port>].
  net::ProxyServer proxy_server =
      net::ProxyServer::FromURI(host, default_proxy_scheme);
  int port = 0;
  onc_proxy_location.GetIntegerWithoutPathExpansion(::onc::proxy::kPort, &port);

  // Replace the port parsed from |host| by the provided |port|.
  return net::ProxyServer(
      proxy_server.scheme(),
      net::HostPortPair(proxy_server.host_port_pair().host(),
                        static_cast<uint16_t>(port)));
}

void AppendProxyServerForScheme(const base::DictionaryValue& onc_manual,
                                const std::string& onc_scheme,
                                std::string* spec) {
  const base::DictionaryValue* onc_proxy_location = nullptr;
  if (!onc_manual.GetDictionaryWithoutPathExpansion(onc_scheme,
                                                    &onc_proxy_location)) {
    return;
  }

  net::ProxyServer::Scheme default_proxy_scheme = net::ProxyServer::SCHEME_HTTP;
  std::string url_scheme;
  if (onc_scheme == ::onc::proxy::kFtp) {
    url_scheme = url::kFtpScheme;
  } else if (onc_scheme == ::onc::proxy::kHttp) {
    url_scheme = url::kHttpScheme;
  } else if (onc_scheme == ::onc::proxy::kHttps) {
    url_scheme = url::kHttpsScheme;
  } else if (onc_scheme == ::onc::proxy::kSocks) {
    default_proxy_scheme = net::ProxyServer::SCHEME_SOCKS4;
    url_scheme = kSocksScheme;
  } else {
    NOTREACHED();
  }

  net::ProxyServer proxy_server = ConvertOncProxyLocationToHostPort(
      default_proxy_scheme, *onc_proxy_location);

  ProxyConfigDictionary::EncodeAndAppendProxyServer(url_scheme, proxy_server,
                                                    spec);
}

net::ProxyBypassRules ConvertOncExcludeDomainsToBypassRules(
    const base::ListValue& onc_exclude_domains) {
  net::ProxyBypassRules rules;
  for (base::ListValue::const_iterator it = onc_exclude_domains.begin();
       it != onc_exclude_domains.end(); ++it) {
    std::string rule;
    it->GetAsString(&rule);
    rules.AddRuleFromString(rule);
  }
  return rules;
}

std::string SchemeToString(net::ProxyServer::Scheme scheme) {
  switch (scheme) {
    case net::ProxyServer::SCHEME_DIRECT:
      return kDirectScheme;
    case net::ProxyServer::SCHEME_HTTP:
      return url::kHttpScheme;
    case net::ProxyServer::SCHEME_SOCKS4:
      return kSocks4Scheme;
    case net::ProxyServer::SCHEME_SOCKS5:
      return kSocks5Scheme;
    case net::ProxyServer::SCHEME_HTTPS:
      return url::kHttpsScheme;
    case net::ProxyServer::SCHEME_QUIC:
      return kQuicScheme;
    case net::ProxyServer::SCHEME_INVALID:
      break;
  }
  NOTREACHED();
  return "";
}

void SetProxyForScheme(const net::ProxyConfig::ProxyRules& proxy_rules,
                       const std::string& scheme,
                       const std::string& onc_scheme,
                       base::DictionaryValue* dict) {
  const net::ProxyList* proxy_list = nullptr;
  if (proxy_rules.type == net::ProxyConfig::ProxyRules::Type::PROXY_LIST) {
    proxy_list = &proxy_rules.single_proxies;
  } else if (proxy_rules.type ==
             net::ProxyConfig::ProxyRules::Type::PROXY_LIST_PER_SCHEME) {
    proxy_list = proxy_rules.MapUrlSchemeToProxyList(scheme);
  }
  if (!proxy_list || proxy_list->IsEmpty())
    return;
  const net::ProxyServer& server = proxy_list->Get();
  std::unique_ptr<base::DictionaryValue> url_dict(new base::DictionaryValue);
  std::string host = server.host_port_pair().host();

  // For all proxy types except SOCKS, the default scheme of the proxy host is
  // HTTP.
  net::ProxyServer::Scheme default_scheme =
      (onc_scheme == ::onc::proxy::kSocks) ? net::ProxyServer::SCHEME_SOCKS4
                                           : net::ProxyServer::SCHEME_HTTP;
  // Only prefix the host with a non-default scheme.
  if (server.scheme() != default_scheme)
    host = SchemeToString(server.scheme()) + "://" + host;
  url_dict->SetKey(::onc::proxy::kHost, base::Value(host));
  url_dict->SetKey(::onc::proxy::kPort,
                   base::Value(server.host_port_pair().port()));
  dict->SetWithoutPathExpansion(onc_scheme, std::move(url_dict));
}

// Returns the NetworkConfiugration with |guid| from |network_configs|, or
// nullptr if no such NetworkConfiguration is found.
const base::DictionaryValue* GetNetworkConfigByGUID(
    const base::ListValue& network_configs,
    const std::string& guid) {
  for (base::ListValue::const_iterator it = network_configs.begin();
       it != network_configs.end(); ++it) {
    const base::DictionaryValue* network = NULL;
    it->GetAsDictionary(&network);
    DCHECK(network);

    std::string current_guid;
    network->GetStringWithoutPathExpansion(::onc::network_config::kGUID,
                                           &current_guid);
    if (current_guid == guid)
      return network;
  }
  return NULL;
}

// Returns the first Ethernet NetworkConfiguration from |network_configs| with
// "Authentication: None", or nullptr if no such NetworkConfiguration is found.
const base::DictionaryValue* GetNetworkConfigForEthernetWithoutEAP(
    const base::ListValue& network_configs) {
  VLOG(2) << "Search for ethernet policy without EAP.";
  for (base::ListValue::const_iterator it = network_configs.begin();
       it != network_configs.end(); ++it) {
    const base::DictionaryValue* network = NULL;
    it->GetAsDictionary(&network);
    DCHECK(network);

    std::string type;
    network->GetStringWithoutPathExpansion(::onc::network_config::kType, &type);
    if (type != ::onc::network_type::kEthernet)
      continue;

    const base::DictionaryValue* ethernet = NULL;
    network->GetDictionaryWithoutPathExpansion(::onc::network_config::kEthernet,
                                               &ethernet);

    std::string auth;
    ethernet->GetStringWithoutPathExpansion(::onc::ethernet::kAuthentication,
                                            &auth);
    if (auth == ::onc::ethernet::kAuthenticationNone)
      return network;
  }
  return NULL;
}

// Returns the NetworkConfiguration object for |network| from
// |network_configs| or nullptr if no matching NetworkConfiguration is found. If
// |network| is a non-Ethernet network, performs a lookup by GUID. If |network|
// is an Ethernet network, tries lookup of the GUID of the shared EthernetEAP
// service, or otherwise returns the first Ethernet NetworkConfiguration with
// "Authentication: None".
const base::DictionaryValue* GetNetworkConfigForNetworkFromOnc(
    const base::ListValue& network_configs,
    const NetworkState& network) {
  // In all cases except Ethernet, we use the GUID of |network|.
  if (!network.Matches(NetworkTypePattern::Ethernet()))
    return GetNetworkConfigByGUID(network_configs, network.guid());

  // Ethernet is always shared and thus cannot store a GUID per user. Thus we
  // search for any Ethernet policy intead of a matching GUID.
  // EthernetEAP service contains only the EAP parameters and stores the GUID of
  // the respective ONC policy. The EthernetEAP service itself is however never
  // in state "connected". An EthernetEAP policy must be applied, if an Ethernet
  // service is connected using the EAP parameters.
  const NetworkState* ethernet_eap = NULL;
  if (NetworkHandler::IsInitialized()) {
    ethernet_eap =
        NetworkHandler::Get()->network_state_handler()->GetEAPForEthernet(
            network.path());
  }

  // The GUID associated with the EthernetEAP service refers to the ONC policy
  // with "Authentication: 8021X".
  if (ethernet_eap)
    return GetNetworkConfigByGUID(network_configs, ethernet_eap->guid());

  // Otherwise, EAP is not used and instead the Ethernet policy with
  // "Authentication: None" applies.
  return GetNetworkConfigForEthernetWithoutEAP(network_configs);
}

// Expects |pref_name| in |pref_service| to be a pref holding an ONC blob.
// Returns the NetworkConfiguration ONC object for |network| from this ONC, or
// nullptr if no configuration is found. See |GetNetworkConfigForNetworkFromOnc|
// for the NetworkConfiguration lookup rules.
const base::DictionaryValue* GetPolicyForNetworkFromPref(
    const PrefService* pref_service,
    const char* pref_name,
    const NetworkState& network) {
  if (!pref_service) {
    VLOG(2) << "No pref service";
    return NULL;
  }

  const PrefService::Preference* preference =
      pref_service->FindPreference(pref_name);
  if (!preference) {
    VLOG(2) << "No preference " << pref_name;
    // The preference may not exist in tests.
    return NULL;
  }

  // User prefs are not stored in this Preference yet but only the policy.
  //
  // The policy server incorrectly configures the OpenNetworkConfiguration user
  // policy as Recommended. To work around that, we handle the Recommended and
  // the Mandatory value in the same way.
  // TODO(pneubeck): Remove this workaround, once the server is fixed. See
  // http://crbug.com/280553 .
  if (preference->IsDefaultValue()) {
    VLOG(2) << "Preference has no recommended or mandatory value.";
    // No policy set.
    return NULL;
  }
  VLOG(2) << "Preference with policy found.";
  const base::Value* onc_policy_value = preference->GetValue();
  DCHECK(onc_policy_value);

  const base::ListValue* onc_policy = NULL;
  onc_policy_value->GetAsList(&onc_policy);
  DCHECK(onc_policy);

  return GetNetworkConfigForNetworkFromOnc(*onc_policy, network);
}

// Returns the global network configuration dictionary from the ONC policy of
// the active user if |for_active_user| is true, or from device policy if it is
// false.
const base::DictionaryValue* GetGlobalConfigFromPolicy(bool for_active_user) {
  std::string username_hash;
  if (for_active_user) {
    const user_manager::User* user =
        user_manager::UserManager::Get()->GetActiveUser();
    if (!user) {
      LOG(ERROR) << "No user logged in yet.";
      return NULL;
    }
    username_hash = user->username_hash();
  }
  return NetworkHandler::Get()
      ->managed_network_configuration_handler()
      ->GetGlobalConfigFromPolicy(username_hash);
}

}  // namespace

const char kEmptyUnencryptedConfiguration[] =
    "{\"Type\":\"UnencryptedConfiguration\",\"NetworkConfigurations\":[],"
    "\"Certificates\":[]}";

std::unique_ptr<base::DictionaryValue> ReadDictionaryFromJson(
    const std::string& json) {
  std::string error;
  std::unique_ptr<base::Value> root = base::JSONReader::ReadAndReturnError(
      json, base::JSON_ALLOW_TRAILING_COMMAS, nullptr, &error);

  base::DictionaryValue* dict_ptr = nullptr;
  if (!root || !root->GetAsDictionary(&dict_ptr)) {
    NET_LOG(ERROR) << "Invalid JSON Dictionary: " << error;
    return nullptr;
  }
  ignore_result(root.release());
  return base::WrapUnique(dict_ptr);
}

std::unique_ptr<base::DictionaryValue> Decrypt(
    const std::string& passphrase,
    const base::DictionaryValue& root) {
  const int kKeySizeInBits = 256;
  const int kMaxIterationCount = 500000;
  std::string onc_type;
  std::string initial_vector;
  std::string salt;
  std::string cipher;
  std::string stretch_method;
  std::string hmac_method;
  std::string hmac;
  int iterations;
  std::string ciphertext;

  if (!root.GetString(encrypted::kCiphertext, &ciphertext) ||
      !root.GetString(encrypted::kCipher, &cipher) ||
      !root.GetString(encrypted::kHMAC, &hmac) ||
      !root.GetString(encrypted::kHMACMethod, &hmac_method) ||
      !root.GetString(encrypted::kIV, &initial_vector) ||
      !root.GetInteger(encrypted::kIterations, &iterations) ||
      !root.GetString(encrypted::kSalt, &salt) ||
      !root.GetString(encrypted::kStretch, &stretch_method) ||
      !root.GetString(toplevel_config::kType, &onc_type) ||
      onc_type != toplevel_config::kEncryptedConfiguration) {
    NET_LOG(ERROR) << "Encrypted ONC malformed.";
    return nullptr;
  }

  if (hmac_method != encrypted::kSHA1 || cipher != encrypted::kAES256 ||
      stretch_method != encrypted::kPBKDF2) {
    NET_LOG(ERROR) << "Encrypted ONC unsupported encryption scheme.";
    return nullptr;
  }

  // Make sure iterations != 0, since that's not valid.
  if (iterations == 0) {
    NET_LOG(ERROR) << kUnableToDecrypt;
    return nullptr;
  }

  // Simply a sanity check to make sure we can't lock up the machine
  // for too long with a huge number (or a negative number).
  if (iterations < 0 || iterations > kMaxIterationCount) {
    NET_LOG(ERROR) << "Too many iterations in encrypted ONC";
    return nullptr;
  }

  if (!base::Base64Decode(salt, &salt)) {
    NET_LOG(ERROR) << kUnableToDecode;
    return nullptr;
  }

  std::unique_ptr<crypto::SymmetricKey> key(
      crypto::SymmetricKey::DeriveKeyFromPassword(crypto::SymmetricKey::AES,
                                                  passphrase, salt, iterations,
                                                  kKeySizeInBits));

  if (!base::Base64Decode(initial_vector, &initial_vector)) {
    NET_LOG(ERROR) << kUnableToDecode;
    return nullptr;
  }
  if (!base::Base64Decode(ciphertext, &ciphertext)) {
    NET_LOG(ERROR) << kUnableToDecode;
    return nullptr;
  }
  if (!base::Base64Decode(hmac, &hmac)) {
    NET_LOG(ERROR) << kUnableToDecode;
    return nullptr;
  }

  crypto::HMAC hmac_verifier(crypto::HMAC::SHA1);
  if (!hmac_verifier.Init(key.get()) ||
      !hmac_verifier.Verify(ciphertext, hmac)) {
    NET_LOG(ERROR) << kUnableToDecrypt;
    return nullptr;
  }

  crypto::Encryptor decryptor;
  if (!decryptor.Init(key.get(), crypto::Encryptor::CBC, initial_vector)) {
    NET_LOG(ERROR) << kUnableToDecrypt;
    return nullptr;
  }

  std::string plaintext;
  if (!decryptor.Decrypt(ciphertext, &plaintext)) {
    NET_LOG(ERROR) << kUnableToDecrypt;
    return nullptr;
  }

  std::unique_ptr<base::DictionaryValue> new_root =
      ReadDictionaryFromJson(plaintext);
  if (!new_root) {
    NET_LOG(ERROR) << "Property dictionary malformed.";
    return nullptr;
  }

  return new_root;
}

std::string GetSourceAsString(ONCSource source) {
  switch (source) {
    case ONC_SOURCE_UNKNOWN:
      return "unknown";
    case ONC_SOURCE_NONE:
      return "none";
    case ONC_SOURCE_DEVICE_POLICY:
      return "device policy";
    case ONC_SOURCE_USER_POLICY:
      return "user policy";
    case ONC_SOURCE_USER_IMPORT:
      return "user import";
  }
  NOTREACHED() << "unknown ONC source " << source;
  return "unknown";
}

void ExpandStringsInOncObject(const OncValueSignature& signature,
                              const VariableExpander& variable_expander,
                              base::DictionaryValue* onc_object) {
  if (&signature == &kEAPSignature) {
    ExpandField(eap::kAnonymousIdentity, variable_expander, onc_object);
    ExpandField(eap::kIdentity, variable_expander, onc_object);
  } else if (&signature == &kL2TPSignature ||
             &signature == &kOpenVPNSignature) {
    ExpandField(vpn::kUsername, variable_expander, onc_object);
  }

  // Recurse into nested objects.
  for (base::DictionaryValue::Iterator it(*onc_object); !it.IsAtEnd();
       it.Advance()) {
    base::DictionaryValue* inner_object = nullptr;
    if (!onc_object->GetDictionaryWithoutPathExpansion(it.key(), &inner_object))
      continue;

    const OncFieldSignature* field_signature =
        GetFieldSignature(signature, it.key());
    if (!field_signature)
      continue;

    ExpandStringsInOncObject(*field_signature->value_signature,
                             variable_expander, inner_object);
  }
}

void ExpandStringsInNetworks(const VariableExpander& variable_expander,
                             base::ListValue* network_configs) {
  for (auto& entry : *network_configs) {
    base::DictionaryValue* network = nullptr;
    entry.GetAsDictionary(&network);
    DCHECK(network);
    ExpandStringsInOncObject(kNetworkConfigurationSignature, variable_expander,
                             network);
  }
}

void FillInHexSSIDFieldsInOncObject(const OncValueSignature& signature,
                                    base::DictionaryValue* onc_object) {
  if (&signature == &kWiFiSignature)
    FillInHexSSIDField(onc_object);

  // Recurse into nested objects.
  for (base::DictionaryValue::Iterator it(*onc_object); !it.IsAtEnd();
       it.Advance()) {
    base::DictionaryValue* inner_object = nullptr;
    if (!onc_object->GetDictionaryWithoutPathExpansion(it.key(), &inner_object))
      continue;

    const OncFieldSignature* field_signature =
        GetFieldSignature(signature, it.key());
    if (!field_signature)
      continue;

    FillInHexSSIDFieldsInOncObject(*field_signature->value_signature,
                                   inner_object);
  }
}

void FillInHexSSIDField(base::DictionaryValue* wifi_fields) {
  std::string ssid_string;
  if (wifi_fields->HasKey(::onc::wifi::kHexSSID) ||
      !wifi_fields->GetStringWithoutPathExpansion(::onc::wifi::kSSID,
                                                  &ssid_string)) {
    return;
  }
  if (ssid_string.empty()) {
    NET_LOG(ERROR) << "Found empty SSID field.";
    return;
  }
  wifi_fields->SetKey(
      ::onc::wifi::kHexSSID,
      base::Value(base::HexEncode(ssid_string.c_str(), ssid_string.size())));
}

std::unique_ptr<base::DictionaryValue> MaskCredentialsInOncObject(
    const OncValueSignature& signature,
    const base::DictionaryValue& onc_object,
    const std::string& mask) {
  return OncMaskValues::Mask(signature, onc_object, mask);
}

std::string DecodePEM(const std::string& pem_encoded) {
  // The PEM block header used for DER certificates
  const char kCertificateHeader[] = "CERTIFICATE";

  // This is an older PEM marker for DER certificates.
  const char kX509CertificateHeader[] = "X509 CERTIFICATE";

  std::vector<std::string> pem_headers;
  pem_headers.push_back(kCertificateHeader);
  pem_headers.push_back(kX509CertificateHeader);

  net::PEMTokenizer pem_tokenizer(pem_encoded, pem_headers);
  std::string decoded;
  if (pem_tokenizer.GetNext()) {
    decoded = pem_tokenizer.data();
  } else {
    // If we failed to read the data as a PEM file, then try plain base64 decode
    // in case the PEM marker strings are missing. For this to work, there has
    // to be no white space, and it has to only contain the base64-encoded data.
    if (!base::Base64Decode(pem_encoded, &decoded)) {
      LOG(ERROR) << "Unable to base64 decode X509 data: " << pem_encoded;
      return std::string();
    }
  }
  return decoded;
}

bool ParseAndValidateOncForImport(const std::string& onc_blob,
                                  ONCSource onc_source,
                                  const std::string& passphrase,
                                  base::ListValue* network_configs,
                                  base::DictionaryValue* global_network_config,
                                  base::ListValue* certificates) {
  if (network_configs)
    network_configs->Clear();
  if (global_network_config)
    global_network_config->Clear();
  if (certificates)
    certificates->Clear();
  if (onc_blob.empty())
    return true;

  std::unique_ptr<base::DictionaryValue> toplevel_onc =
      ReadDictionaryFromJson(onc_blob);
  if (!toplevel_onc) {
    LOG(ERROR) << "ONC loaded from " << GetSourceAsString(onc_source)
               << " is not a valid JSON dictionary.";
    return false;
  }

  // Check and see if this is an encrypted ONC file. If so, decrypt it.
  std::string onc_type;
  toplevel_onc->GetStringWithoutPathExpansion(toplevel_config::kType,
                                              &onc_type);
  if (onc_type == toplevel_config::kEncryptedConfiguration) {
    toplevel_onc = Decrypt(passphrase, *toplevel_onc);
    if (!toplevel_onc) {
      LOG(ERROR) << "Couldn't decrypt the ONC from "
                 << GetSourceAsString(onc_source);
      return false;
    }
  }

  bool from_policy = (onc_source == ONC_SOURCE_USER_POLICY ||
                      onc_source == ONC_SOURCE_DEVICE_POLICY);

  // Validate the ONC dictionary. We are liberal and ignore unknown field
  // names and ignore invalid field names in kRecommended arrays.
  Validator validator(false,  // Ignore unknown fields.
                      false,  // Ignore invalid recommended field names.
                      true,   // Fail on missing fields.
                      from_policy);
  validator.SetOncSource(onc_source);

  Validator::Result validation_result;
  toplevel_onc = validator.ValidateAndRepairObject(
      &kToplevelConfigurationSignature, *toplevel_onc, &validation_result);

  if (from_policy) {
    UMA_HISTOGRAM_BOOLEAN("Enterprise.ONC.PolicyValidation",
                          validation_result == Validator::VALID);
  }

  bool success = true;
  if (validation_result == Validator::VALID_WITH_WARNINGS) {
    LOG(WARNING) << "ONC from " << GetSourceAsString(onc_source)
                 << " produced warnings.";
    success = false;
  } else if (validation_result == Validator::INVALID || !toplevel_onc) {
    LOG(ERROR) << "ONC from " << GetSourceAsString(onc_source)
               << " is invalid and couldn't be repaired.";
    return false;
  }

  base::ListValue* validated_certs = nullptr;
  if (certificates && toplevel_onc->GetListWithoutPathExpansion(
                          toplevel_config::kCertificates, &validated_certs)) {
    certificates->Swap(validated_certs);
  }

  base::ListValue* validated_networks = nullptr;
  // Note that this processing is performed even if |network_configs| is
  // nullptr, because ResolveServerCertRefsInNetworks could affect the return
  // value of the function (which is supposed to aggregate validation issues in
  // all segments of the ONC blob).
  if (toplevel_onc->GetListWithoutPathExpansion(
          toplevel_config::kNetworkConfigurations, &validated_networks)) {
    FillInHexSSIDFieldsInNetworks(validated_networks);

    CertPEMsByGUIDMap server_and_ca_certs =
        GetServerAndCACertsByGUID(*certificates);

    if (!ResolveServerCertRefsInNetworks(server_and_ca_certs,
                                         validated_networks)) {
      LOG(ERROR) << "Some certificate references in the ONC policy for source "
                 << GetSourceAsString(onc_source) << " could not be resolved.";
      success = false;
    }

    if (network_configs)
      network_configs->Swap(validated_networks);
  }

  base::DictionaryValue* validated_global_config = nullptr;
  if (global_network_config && toplevel_onc->GetDictionaryWithoutPathExpansion(
                                   toplevel_config::kGlobalNetworkConfiguration,
                                   &validated_global_config)) {
    global_network_config->Swap(validated_global_config);
  }

  return success;
}

net::ScopedCERTCertificate DecodePEMCertificate(
    const std::string& pem_encoded) {
  std::string decoded = DecodePEM(pem_encoded);
  net::ScopedCERTCertificate cert =
      net::x509_util::CreateCERTCertificateFromBytes(
          reinterpret_cast<const uint8_t*>(decoded.data()), decoded.size());
  LOG_IF(ERROR, !cert.get())
      << "Couldn't create certificate from X509 data: " << decoded;
  return cert;
}

bool ResolveServerCertRefsInNetworks(const CertPEMsByGUIDMap& certs_by_guid,
                                     base::ListValue* network_configs) {
  bool success = true;
  for (base::ListValue::iterator it = network_configs->begin();
       it != network_configs->end();) {
    base::DictionaryValue* network = nullptr;
    it->GetAsDictionary(&network);
    if (!ResolveServerCertRefsInNetwork(certs_by_guid, network)) {
      std::string guid;
      network->GetStringWithoutPathExpansion(network_config::kGUID, &guid);
      // This might happen even with correct validation, if the referenced
      // certificate couldn't be imported.
      LOG(ERROR) << "Couldn't resolve some certificate reference of network "
                 << guid;
      it = network_configs->Erase(it, nullptr);
      success = false;
      continue;
    }
    ++it;
  }
  return success;
}

bool ResolveServerCertRefsInNetwork(const CertPEMsByGUIDMap& certs_by_guid,
                                    base::DictionaryValue* network_config) {
  return ResolveServerCertRefsInObject(
      certs_by_guid, kNetworkConfigurationSignature, network_config);
}

NetworkTypePattern NetworkTypePatternFromOncType(const std::string& type) {
  if (type == ::onc::network_type::kAllTypes)
    return NetworkTypePattern::Default();
  if (type == ::onc::network_type::kCellular)
    return NetworkTypePattern::Cellular();
  if (type == ::onc::network_type::kEthernet)
    return NetworkTypePattern::Ethernet();
  if (type == ::onc::network_type::kTether)
    return NetworkTypePattern::Tether();
  if (type == ::onc::network_type::kVPN)
    return NetworkTypePattern::VPN();
  if (type == ::onc::network_type::kWiFi)
    return NetworkTypePattern::WiFi();
  if (type == ::onc::network_type::kWimax)
    return NetworkTypePattern::Wimax();
  if (type == ::onc::network_type::kWireless)
    return NetworkTypePattern::Wireless();
  NOTREACHED() << "Unrecognized ONC type: " << type;
  return NetworkTypePattern::Default();
}

bool IsRecommendedValue(const base::DictionaryValue* onc,
                        const std::string& property_key) {
  std::string property_basename, recommended_property_key;
  size_t pos = property_key.find_last_of('.');
  if (pos != std::string::npos) {
    // 'WiFi.AutoConnect' -> 'AutoConnect', 'WiFi.Recommended'
    property_basename = property_key.substr(pos + 1);
    recommended_property_key =
        property_key.substr(0, pos + 1) + ::onc::kRecommended;
  } else {
    // 'Name' -> 'Name', 'Recommended'
    property_basename = property_key;
    recommended_property_key = ::onc::kRecommended;
  }

  const base::ListValue* recommended_keys = nullptr;
  return (onc->GetList(recommended_property_key, &recommended_keys) &&
          recommended_keys->Find(base::Value(property_basename)) !=
              recommended_keys->end());
}

std::unique_ptr<base::DictionaryValue> ConvertOncProxySettingsToProxyConfig(
    const base::DictionaryValue& onc_proxy_settings) {
  std::string type;
  onc_proxy_settings.GetStringWithoutPathExpansion(::onc::proxy::kType, &type);
  std::unique_ptr<base::DictionaryValue> proxy_dict;

  if (type == ::onc::proxy::kDirect) {
    proxy_dict = ProxyConfigDictionary::CreateDirect();
  } else if (type == ::onc::proxy::kWPAD) {
    proxy_dict = ProxyConfigDictionary::CreateAutoDetect();
  } else if (type == ::onc::proxy::kPAC) {
    std::string pac_url;
    onc_proxy_settings.GetStringWithoutPathExpansion(::onc::proxy::kPAC,
                                                     &pac_url);
    GURL url(url_formatter::FixupURL(pac_url, std::string()));
    proxy_dict = ProxyConfigDictionary::CreatePacScript(
        url.is_valid() ? url.spec() : std::string(), false);
  } else if (type == ::onc::proxy::kManual) {
    const base::DictionaryValue* manual_dict = nullptr;
    onc_proxy_settings.GetDictionaryWithoutPathExpansion(::onc::proxy::kManual,
                                                         &manual_dict);
    std::string manual_spec;
    AppendProxyServerForScheme(*manual_dict, ::onc::proxy::kFtp, &manual_spec);
    AppendProxyServerForScheme(*manual_dict, ::onc::proxy::kHttp, &manual_spec);
    AppendProxyServerForScheme(*manual_dict, ::onc::proxy::kSocks,
                               &manual_spec);
    AppendProxyServerForScheme(*manual_dict, ::onc::proxy::kHttps,
                               &manual_spec);

    const base::ListValue* exclude_domains = nullptr;
    net::ProxyBypassRules bypass_rules;
    if (onc_proxy_settings.GetListWithoutPathExpansion(
            ::onc::proxy::kExcludeDomains, &exclude_domains)) {
      bypass_rules.AssignFrom(
          ConvertOncExcludeDomainsToBypassRules(*exclude_domains));
    }
    proxy_dict = ProxyConfigDictionary::CreateFixedServers(
        manual_spec, bypass_rules.ToString());
  } else {
    NOTREACHED();
  }
  return proxy_dict;
}

std::unique_ptr<base::DictionaryValue> ConvertProxyConfigToOncProxySettings(
    std::unique_ptr<base::DictionaryValue> proxy_config_value) {
  // Create a ProxyConfigDictionary from the DictionaryValue.
  auto proxy_config =
      std::make_unique<ProxyConfigDictionary>(std::move(proxy_config_value));

  // Create the result DictionaryValue and populate it.
  std::unique_ptr<base::DictionaryValue> proxy_settings(
      new base::DictionaryValue);
  ProxyPrefs::ProxyMode mode;
  if (!proxy_config->GetMode(&mode))
    return nullptr;
  switch (mode) {
    case ProxyPrefs::MODE_DIRECT: {
      proxy_settings->SetKey(::onc::proxy::kType,
                             base::Value(::onc::proxy::kDirect));
      break;
    }
    case ProxyPrefs::MODE_AUTO_DETECT: {
      proxy_settings->SetKey(::onc::proxy::kType,
                             base::Value(::onc::proxy::kWPAD));
      break;
    }
    case ProxyPrefs::MODE_PAC_SCRIPT: {
      proxy_settings->SetKey(::onc::proxy::kType,
                             base::Value(::onc::proxy::kPAC));
      std::string pac_url;
      proxy_config->GetPacUrl(&pac_url);
      proxy_settings->SetKey(::onc::proxy::kPAC, base::Value(pac_url));
      break;
    }
    case ProxyPrefs::MODE_FIXED_SERVERS: {
      proxy_settings->SetString(::onc::proxy::kType, ::onc::proxy::kManual);
      std::unique_ptr<base::DictionaryValue> manual(new base::DictionaryValue);
      std::string proxy_rules_string;
      if (proxy_config->GetProxyServer(&proxy_rules_string)) {
        net::ProxyConfig::ProxyRules proxy_rules;
        proxy_rules.ParseFromString(proxy_rules_string);
        SetProxyForScheme(proxy_rules, url::kFtpScheme, ::onc::proxy::kFtp,
                          manual.get());
        SetProxyForScheme(proxy_rules, url::kHttpScheme, ::onc::proxy::kHttp,
                          manual.get());
        SetProxyForScheme(proxy_rules, url::kHttpsScheme, ::onc::proxy::kHttps,
                          manual.get());
        SetProxyForScheme(proxy_rules, kSocksScheme, ::onc::proxy::kSocks,
                          manual.get());
      }
      proxy_settings->SetWithoutPathExpansion(::onc::proxy::kManual,
                                              std::move(manual));

      // Convert the 'bypass_list' string into dictionary entries.
      std::string bypass_rules_string;
      if (proxy_config->GetBypassList(&bypass_rules_string)) {
        net::ProxyBypassRules bypass_rules;
        bypass_rules.ParseFromString(bypass_rules_string);
        std::unique_ptr<base::ListValue> exclude_domains(new base::ListValue);
        for (const auto& rule : bypass_rules.rules())
          exclude_domains->AppendString(rule->ToString());
        if (!exclude_domains->empty()) {
          proxy_settings->SetWithoutPathExpansion(::onc::proxy::kExcludeDomains,
                                                  std::move(exclude_domains));
        }
      }
      break;
    }
    default: {
      LOG(ERROR) << "Unexpected proxy mode in Shill config: " << mode;
      return nullptr;
    }
  }
  return proxy_settings;
}

void ExpandStringPlaceholdersInNetworksForUser(
    const user_manager::User* user,
    base::ListValue* network_configs) {
  if (!user) {
    // In tests no user may be logged in. It's not harmful if we just don't
    // expand the strings.
    return;
  }

  // Note: It is OK for the placeholders to be replaced with empty strings if
  // that is what the getters on |user| provide.
  std::map<std::string, std::string> substitutions;
  substitutions[::onc::substitutes::kLoginID] = user->GetAccountName(false);
  substitutions[::onc::substitutes::kLoginEmail] =
      user->GetAccountId().GetUserEmail();
  VariableExpander variable_expander(std::move(substitutions));
  chromeos::onc::ExpandStringsInNetworks(variable_expander, network_configs);
}

void ImportNetworksForUser(const user_manager::User* user,
                           const base::ListValue& network_configs,
                           std::string* error) {
  error->clear();

  std::unique_ptr<base::ListValue> expanded_networks(
      network_configs.DeepCopy());
  ExpandStringPlaceholdersInNetworksForUser(user, expanded_networks.get());

  const NetworkProfile* profile =
      NetworkHandler::Get()->network_profile_handler()->GetProfileForUserhash(
          user->username_hash());
  if (!profile) {
    *error = "User profile doesn't exist.";
    return;
  }

  bool ethernet_not_found = false;
  for (base::ListValue::const_iterator it = expanded_networks->begin();
       it != expanded_networks->end(); ++it) {
    const base::DictionaryValue* network = NULL;
    it->GetAsDictionary(&network);
    DCHECK(network);

    // Remove irrelevant fields.
    onc::Normalizer normalizer(true /* remove recommended fields */);
    std::unique_ptr<base::DictionaryValue> normalized_network =
        normalizer.NormalizeObject(&onc::kNetworkConfigurationSignature,
                                   *network);

    // TODO(pneubeck): Use ONC and ManagedNetworkConfigurationHandler instead.
    // crbug.com/457936
    std::unique_ptr<base::DictionaryValue> shill_dict =
        onc::TranslateONCObjectToShill(&onc::kNetworkConfigurationSignature,
                                       *normalized_network);

    std::unique_ptr<NetworkUIData> ui_data(
        NetworkUIData::CreateFromONC(::onc::ONC_SOURCE_USER_IMPORT));
    base::DictionaryValue ui_data_dict;
    ui_data->FillDictionary(&ui_data_dict);
    std::string ui_data_json;
    base::JSONWriter::Write(ui_data_dict, &ui_data_json);
    shill_dict->SetKey(shill::kUIDataProperty, base::Value(ui_data_json));

    shill_dict->SetKey(shill::kProfileProperty, base::Value(profile->path));

    std::string type;
    shill_dict->GetStringWithoutPathExpansion(shill::kTypeProperty, &type);
    NetworkConfigurationHandler* config_handler =
        NetworkHandler::Get()->network_configuration_handler();
    if (NetworkTypePattern::Ethernet().MatchesType(type)) {
      // Ethernet has to be configured using an existing Ethernet service.
      const NetworkState* ethernet =
          NetworkHandler::Get()->network_state_handler()->FirstNetworkByType(
              NetworkTypePattern::Ethernet());
      if (ethernet) {
        config_handler->SetShillProperties(
            ethernet->path(), *shill_dict,
            NetworkConfigurationObserver::SOURCE_USER_ACTION, base::Closure(),
            network_handler::ErrorCallback());
      } else {
        ethernet_not_found = true;
      }

    } else {
      config_handler->CreateShillConfiguration(
          *shill_dict, NetworkConfigurationObserver::SOURCE_USER_ACTION,
          network_handler::ServiceResultCallback(),
          network_handler::ErrorCallback());
    }
  }

  if (ethernet_not_found)
    *error = "No Ethernet available to configure.";
}

const base::DictionaryValue* FindPolicyForActiveUser(
    const std::string& guid,
    ::onc::ONCSource* onc_source) {
  const user_manager::User* user =
      user_manager::UserManager::Get()->GetActiveUser();
  std::string username_hash = user ? user->username_hash() : std::string();
  return NetworkHandler::Get()
      ->managed_network_configuration_handler()
      ->FindPolicyByGUID(username_hash, guid, onc_source);
}

bool PolicyAllowsOnlyPolicyNetworksToAutoconnect(bool for_active_user) {
  const base::DictionaryValue* global_config =
      GetGlobalConfigFromPolicy(for_active_user);
  if (!global_config)
    return false;  // By default, all networks are allowed to autoconnect.

  bool only_policy_autoconnect = false;
  global_config->GetBooleanWithoutPathExpansion(
      ::onc::global_network_config::kAllowOnlyPolicyNetworksToAutoconnect,
      &only_policy_autoconnect);
  return only_policy_autoconnect;
}

const base::DictionaryValue* GetPolicyForNetwork(
    const PrefService* profile_prefs,
    const PrefService* local_state_prefs,
    const NetworkState& network,
    ::onc::ONCSource* onc_source) {
  VLOG(2) << "GetPolicyForNetwork: " << network.path();
  *onc_source = ::onc::ONC_SOURCE_NONE;

  const base::DictionaryValue* network_policy = GetPolicyForNetworkFromPref(
      profile_prefs, ::onc::prefs::kOpenNetworkConfiguration, network);
  if (network_policy) {
    VLOG(1) << "Network " << network.path() << " is managed by user policy.";
    *onc_source = ::onc::ONC_SOURCE_USER_POLICY;
    return network_policy;
  }
  network_policy = GetPolicyForNetworkFromPref(
      local_state_prefs, ::onc::prefs::kDeviceOpenNetworkConfiguration,
      network);
  if (network_policy) {
    VLOG(1) << "Network " << network.path() << " is managed by device policy.";
    *onc_source = ::onc::ONC_SOURCE_DEVICE_POLICY;
    return network_policy;
  }
  VLOG(2) << "Network " << network.path() << " is unmanaged.";
  return NULL;
}

bool HasPolicyForNetwork(const PrefService* profile_prefs,
                         const PrefService* local_state_prefs,
                         const NetworkState& network) {
  ::onc::ONCSource ignored_onc_source;
  const base::DictionaryValue* policy = onc::GetPolicyForNetwork(
      profile_prefs, local_state_prefs, network, &ignored_onc_source);
  return policy != NULL;
}

bool HasUserPasswordSubsitutionVariable(const OncValueSignature& signature,
                                        base::DictionaryValue* onc_object) {
  if (&signature == &kEAPSignature) {
    std::string password_field;
    if (!onc_object->GetStringWithoutPathExpansion(::onc::eap::kPassword,
                                                   &password_field)) {
      return false;
    }

    if (password_field == ::onc::substitutes::kPasswordPlaceholderVerbatim) {
      return true;
    }
  }

  // Recurse into nested objects.
  for (base::DictionaryValue::Iterator it(*onc_object); !it.IsAtEnd();
       it.Advance()) {
    base::DictionaryValue* inner_object = nullptr;
    if (!onc_object->GetDictionaryWithoutPathExpansion(it.key(), &inner_object))
      continue;

    const OncFieldSignature* field_signature =
        GetFieldSignature(signature, it.key());
    if (!field_signature)
      continue;

    bool result = HasUserPasswordSubsitutionVariable(
        *field_signature->value_signature, inner_object);
    if (result) {
      return true;
    }
  }

  return false;
}

bool HasUserPasswordSubsitutionVariable(base::ListValue* network_configs) {
  for (auto& entry : *network_configs) {
    base::DictionaryValue* network = nullptr;
    entry.GetAsDictionary(&network);
    DCHECK(network);

    bool result = HasUserPasswordSubsitutionVariable(
        kNetworkConfigurationSignature, network);
    if (result) {
      return true;
    }
  }
  return false;
}

}  // namespace onc
}  // namespace chromeos
