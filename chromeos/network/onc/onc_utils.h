// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_ONC_ONC_UTILS_H_
#define CHROMEOS_NETWORK_ONC_ONC_UTILS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/network/network_type_pattern.h"
#include "chromeos/tools/variable_expander.h"
#include "components/onc/onc_constants.h"
#include "net/cert/scoped_nss_types.h"

class PrefService;

namespace base {
class DictionaryValue;
class ListValue;
}

namespace user_manager {
class User;
}

namespace chromeos {

class NetworkState;

namespace onc {

struct OncValueSignature;

// A valid but empty (no networks and no certificates) and unencrypted
// configuration.
CHROMEOS_EXPORT extern const char kEmptyUnencryptedConfiguration[];

typedef std::map<std::string, std::string> CertPEMsByGUIDMap;

// Parses |json| according to the JSON format. If |json| is a JSON formatted
// dictionary, the function returns the dictionary as a DictionaryValue.
// Otherwise returns NULL.
CHROMEOS_EXPORT std::unique_ptr<base::DictionaryValue> ReadDictionaryFromJson(
    const std::string& json);

// Decrypts the given EncryptedConfiguration |onc| (see the ONC specification)
// using |passphrase|. The resulting UnencryptedConfiguration is returned. If an
// error occurs, returns NULL.
CHROMEOS_EXPORT std::unique_ptr<base::DictionaryValue> Decrypt(
    const std::string& passphrase,
    const base::DictionaryValue& onc);

// For logging only: strings not user facing.
CHROMEOS_EXPORT std::string GetSourceAsString(::onc::ONCSource source);

// Replaces all expandable fields that are mentioned in the ONC
// specification. The object of |onc_object| is modified in place.
// The substitution is performed using the passed |variable_expander|, which
// defines the placeholder-value mapping.
CHROMEOS_EXPORT void ExpandStringsInOncObject(
    const OncValueSignature& signature,
    const VariableExpander& variable_expander,
    base::DictionaryValue* onc_object);

// Replaces expandable fields in the networks of |network_configs|, which must
// be a list of ONC NetworkConfigurations. See ExpandStringsInOncObject above.
CHROMEOS_EXPORT void ExpandStringsInNetworks(
    const VariableExpander& variable_expander,
    base::ListValue* network_configs);

// Fills in all missing HexSSID fields that are mentioned in the ONC
// specification. The object of |onc_object| is modified in place.
CHROMEOS_EXPORT void FillInHexSSIDFieldsInOncObject(
    const OncValueSignature& signature,
    base::DictionaryValue* onc_object);

// If the SSID field is set, but HexSSID is not, converts the contents of the
// SSID field to UTF-8 encoding, creates the hex representation and assigns the
// result to HexSSID.
CHROMEOS_EXPORT void FillInHexSSIDField(base::DictionaryValue* wifi_fields);

// Creates a copy of |onc_object| with all values of sensitive fields replaced
// by |mask|. To find sensitive fields, signature and field name are checked
// with the function FieldIsCredential().
CHROMEOS_EXPORT std::unique_ptr<base::DictionaryValue>
MaskCredentialsInOncObject(const OncValueSignature& signature,
                           const base::DictionaryValue& onc_object,
                           const std::string& mask);

// Decrypts |onc_blob| with |passphrase| if necessary. Clears |network_configs|,
// |global_network_config| and |certificates| and fills them with the validated
// NetworkConfigurations, GlobalNetworkConfiguration and Certificates of
// |onc_blob|. Callers can pass nullptr as any of |network_configs|,
// |global_network_config|, |certificates| if they're not interested in the
// respective values. Returns false if any validation errors or warnings
// occurred in any segments (i.e. not only those requested by the caller). Even
// if false is returned, some configuration might be added to the output
// arguments and should be further processed by the caller.
CHROMEOS_EXPORT bool ParseAndValidateOncForImport(
    const std::string& onc_blob,
    ::onc::ONCSource onc_source,
    const std::string& passphrase,
    base::ListValue* network_configs,
    base::DictionaryValue* global_network_config,
    base::ListValue* certificates);

// Parse the given PEM encoded certificate |pem_encoded| and return the
// contained DER encoding. Returns an empty string on failure.
std::string DecodePEM(const std::string& pem_encoded);

// Parse the given PEM encoded certificate |pem_encoded| and create a
// CERTCertificate from it.
CHROMEOS_EXPORT net::ScopedCERTCertificate DecodePEMCertificate(
    const std::string& pem_encoded);

// Replaces all references by GUID to Server or CA certs by their PEM
// encoding. Returns true if all references could be resolved. Otherwise returns
// false and network configurations with unresolveable references are removed
// from |network_configs|. |network_configs| must be a list of ONC
// NetworkConfiguration dictionaries.
CHROMEOS_EXPORT bool ResolveServerCertRefsInNetworks(
    const CertPEMsByGUIDMap& certs_by_guid,
    base::ListValue* network_configs);

// Replaces all references by GUID to Server or CA certs by their PEM
// encoding. Returns true if all references could be resolved. |network_config|
// must be a ONC NetworkConfiguration.
CHROMEOS_EXPORT bool ResolveServerCertRefsInNetwork(
    const CertPEMsByGUIDMap& certs_by_guid,
    base::DictionaryValue* network_config);

// Returns a network type pattern for matching the ONC type string.
CHROMEOS_EXPORT NetworkTypePattern NetworkTypePatternFromOncType(
    const std::string& type);

// Returns true if |property_key| is a recommended value in the ONC dictionary.
CHROMEOS_EXPORT bool IsRecommendedValue(const base::DictionaryValue* onc,
                                        const std::string& property_key);

// Translates |onc_proxy_settings|, which must be a valid ONC ProxySettings
// dictionary, to a ProxyConfig dictionary (see proxy_config_dictionary.h).
CHROMEOS_EXPORT std::unique_ptr<base::DictionaryValue>
ConvertOncProxySettingsToProxyConfig(
    const base::DictionaryValue& onc_proxy_settings);

// Translates |proxy_config_value|, which must be a valid ProxyConfig dictionary
// (see proxy_config_dictionary.h) to an ONC ProxySettings dictionary.
CHROMEOS_EXPORT std::unique_ptr<base::DictionaryValue>
ConvertProxyConfigToOncProxySettings(
    std::unique_ptr<base::DictionaryValue> proxy_config_value);

// Replaces user-specific string placeholders in |network_configs|, which must
// be a list of ONC NetworkConfigurations. Currently only user name placeholders
// are implemented, which are replaced by attributes from |user|.
CHROMEOS_EXPORT void ExpandStringPlaceholdersInNetworksForUser(
    const user_manager::User* user,
    base::ListValue* network_configs);

CHROMEOS_EXPORT void ImportNetworksForUser(
    const user_manager::User* user,
    const base::ListValue& network_configs,
    std::string* error);

// Looks up the policy for |guid| for the current active user and sets
// |global_config| (if not NULL) and |onc_source| (if not NULL) accordingly. If
// |guid| is empty, returns NULL and sets the |global_config| and |onc_source|
// if a policy is found.
CHROMEOS_EXPORT const base::DictionaryValue* FindPolicyForActiveUser(
    const std::string& guid,
    ::onc::ONCSource* onc_source);

// Convenvience function to retrieve the "AllowOnlyPolicyNetworksToAutoconnect"
// setting from the global network configuration (see
// GetGlobalConfigFromPolicy).
CHROMEOS_EXPORT bool PolicyAllowsOnlyPolicyNetworksToAutoconnect(
    bool for_active_user);

// Returns the effective (user or device) policy for network |network|. Both
// |profile_prefs| and |local_state_prefs| might be NULL. Returns NULL if no
// applicable policy is found. Sets |onc_source| accordingly.
CHROMEOS_EXPORT const base::DictionaryValue* GetPolicyForNetwork(
    const PrefService* profile_prefs,
    const PrefService* local_state_prefs,
    const NetworkState& network,
    ::onc::ONCSource* onc_source);

// Convenience function to check only whether a policy for a network exists.
CHROMEOS_EXPORT bool HasPolicyForNetwork(const PrefService* profile_prefs,
                                         const PrefService* local_state_prefs,
                                         const NetworkState& network);

// Checks whether a WiFi dictionary object has the ${PASSWORD} substitution
// variable set as the password.
CHROMEOS_EXPORT bool HasUserPasswordSubsitutionVariable(
    const OncValueSignature& signature,
    base::DictionaryValue* onc_object);

// Checks whether a list of network objects has at least one network with the
// ${PASSWORD} substitution variable set as the password.
CHROMEOS_EXPORT bool HasUserPasswordSubsitutionVariable(
    base::ListValue* network_configs);

}  // namespace onc
}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_ONC_ONC_UTILS_H_
