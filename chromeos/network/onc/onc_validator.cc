// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/onc/onc_validator.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <utility>

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "chromeos/network/onc/onc_signature.h"

namespace chromeos {
namespace onc {

namespace {

// According to the IEEE 802.11 standard the SSID is a series of 0 to 32 octets.
const int kMaximumSSIDLengthInBytes = 32;

template <typename T, size_t N>
std::vector<T> toVector(T const (&array)[N]) {
  return std::vector<T>(array, array + N);
}

void AddKeyToList(const char* key, base::Value::ListStorage& list) {
  base::Value key_value(key);
  if (!base::ContainsValue(list, key_value))
    list.push_back(std::move(key_value));
}

std::string GetStringFromDict(const base::Value& dict, const char* key) {
  const base::Value* value = dict.FindKeyOfType(key, base::Value::Type::STRING);
  return value ? value->GetString() : std::string();
}

}  // namespace

Validator::Validator(bool error_on_unknown_field,
                     bool error_on_wrong_recommended,
                     bool error_on_missing_field,
                     bool managed_onc)
    : error_on_unknown_field_(error_on_unknown_field),
      error_on_wrong_recommended_(error_on_wrong_recommended),
      error_on_missing_field_(error_on_missing_field),
      managed_onc_(managed_onc),
      onc_source_(::onc::ONC_SOURCE_NONE) {}

Validator::~Validator() = default;

std::unique_ptr<base::DictionaryValue> Validator::ValidateAndRepairObject(
    const OncValueSignature* object_signature,
    const base::Value& onc_object,
    Result* result) {
  CHECK(object_signature);
  *result = VALID;
  error_or_warning_found_ = false;
  bool error = false;
  std::unique_ptr<base::Value> result_value =
      MapValue(*object_signature, onc_object, &error);
  if (error) {
    *result = INVALID;
    result_value.reset();
  } else if (error_or_warning_found_) {
    *result = VALID_WITH_WARNINGS;
  }
  // The return value should be NULL if, and only if, |result| equals INVALID.
  DCHECK_EQ(!result_value, *result == INVALID);
  return base::DictionaryValue::From(std::move(result_value));
}

std::unique_ptr<base::Value> Validator::MapValue(
    const OncValueSignature& signature,
    const base::Value& onc_value,
    bool* error) {
  if (onc_value.type() != signature.onc_type) {
    LOG(ERROR) << MessageHeader() << "Found value '" << onc_value
               << "' of type '" << base::Value::GetTypeName(onc_value.type())
               << "', but type '"
               << base::Value::GetTypeName(signature.onc_type)
               << "' is required.";
    error_or_warning_found_ = *error = true;
    return std::unique_ptr<base::Value>();
  }

  std::unique_ptr<base::Value> repaired =
      Mapper::MapValue(signature, onc_value, error);
  if (repaired)
    CHECK_EQ(repaired->type(), signature.onc_type);
  return repaired;
}

std::unique_ptr<base::DictionaryValue> Validator::MapObject(
    const OncValueSignature& signature,
    const base::DictionaryValue& onc_object,
    bool* error) {
  std::unique_ptr<base::DictionaryValue> repaired(new base::DictionaryValue);

  bool valid = ValidateObjectDefault(signature, onc_object, repaired.get());
  if (valid) {
    if (&signature == &kToplevelConfigurationSignature) {
      valid = ValidateToplevelConfiguration(repaired.get());
    } else if (&signature == &kNetworkConfigurationSignature) {
      valid = ValidateNetworkConfiguration(repaired.get());
    } else if (&signature == &kEthernetSignature) {
      valid = ValidateEthernet(repaired.get());
    } else if (&signature == &kIPConfigSignature ||
               &signature == &kSavedIPConfigSignature ||
               &signature == &kStaticIPConfigSignature) {
      valid = ValidateIPConfig(repaired.get());
    } else if (&signature == &kWiFiSignature) {
      valid = ValidateWiFi(repaired.get());
    } else if (&signature == &kVPNSignature) {
      valid = ValidateVPN(repaired.get());
    } else if (&signature == &kIPsecSignature) {
      valid = ValidateIPsec(repaired.get());
    } else if (&signature == &kOpenVPNSignature) {
      valid = ValidateOpenVPN(repaired.get());
    } else if (&signature == &kThirdPartyVPNSignature) {
      valid = ValidateThirdPartyVPN(repaired.get());
    } else if (&signature == &kARCVPNSignature) {
      valid = ValidateARCVPN(repaired.get());
    } else if (&signature == &kVerifyX509Signature) {
      valid = ValidateVerifyX509(repaired.get());
    } else if (&signature == &kCertificatePatternSignature) {
      valid = ValidateCertificatePattern(repaired.get());
    } else if (&signature == &kGlobalNetworkConfigurationSignature) {
      valid = ValidateGlobalNetworkConfiguration(repaired.get());
    } else if (&signature == &kProxySettingsSignature) {
      valid = ValidateProxySettings(repaired.get());
    } else if (&signature == &kProxyLocationSignature) {
      valid = ValidateProxyLocation(repaired.get());
    } else if (&signature == &kEAPSignature) {
      valid = ValidateEAP(repaired.get());
    } else if (&signature == &kCertificateSignature) {
      valid = ValidateCertificate(repaired.get());
    } else if (&signature == &kTetherWithStateSignature) {
      valid = ValidateTether(repaired.get());
    }
  }

  if (valid)
    return repaired;

  DCHECK(error_or_warning_found_);
  error_or_warning_found_ = *error = true;
  return std::unique_ptr<base::DictionaryValue>();
}

std::unique_ptr<base::Value> Validator::MapField(
    const std::string& field_name,
    const OncValueSignature& object_signature,
    const base::Value& onc_value,
    bool* found_unknown_field,
    bool* error) {
  path_.push_back(field_name);
  bool current_field_unknown = false;
  std::unique_ptr<base::Value> result = Mapper::MapField(
      field_name, object_signature, onc_value, &current_field_unknown, error);

  DCHECK_EQ(field_name, path_.back());
  path_.pop_back();

  if (current_field_unknown) {
    error_or_warning_found_ = *found_unknown_field = true;
    std::string message = MessageHeader() + "Field name '" + field_name +
        "' is unknown.";
    if (error_on_unknown_field_)
      LOG(ERROR) << message;
    else
      LOG(WARNING) << message;
  }

  return result;
}

std::unique_ptr<base::ListValue> Validator::MapArray(
    const OncValueSignature& array_signature,
    const base::ListValue& onc_array,
    bool* nested_error) {
  bool nested_error_in_current_array = false;
  std::unique_ptr<base::ListValue> result = Mapper::MapArray(
      array_signature, onc_array, &nested_error_in_current_array);

  // Drop individual networks and certificates instead of rejecting all of
  // the configuration.
  if (nested_error_in_current_array &&
      &array_signature != &kNetworkConfigurationListSignature &&
      &array_signature != &kCertificateListSignature) {
    *nested_error = nested_error_in_current_array;
  }
  return result;
}

std::unique_ptr<base::Value> Validator::MapEntry(
    int index,
    const OncValueSignature& signature,
    const base::Value& onc_value,
    bool* error) {
  std::string str = base::IntToString(index);
  path_.push_back(str);
  std::unique_ptr<base::Value> result =
      Mapper::MapEntry(index, signature, onc_value, error);
  DCHECK_EQ(str, path_.back());
  path_.pop_back();
  return result;
}

bool Validator::ValidateObjectDefault(const OncValueSignature& signature,
                                      const base::DictionaryValue& onc_object,
                                      base::DictionaryValue* result) {
  bool found_unknown_field = false;
  bool nested_error_occured = false;
  MapFields(signature, onc_object, &found_unknown_field, &nested_error_occured,
            result);

  if (found_unknown_field && error_on_unknown_field_) {
    DVLOG(1) << "Unknown field names are errors: Aborting.";
    return false;
  }

  if (nested_error_occured)
    return false;

  return ValidateRecommendedField(signature, result);
}

bool Validator::ValidateRecommendedField(
    const OncValueSignature& object_signature,
    base::DictionaryValue* result) {
  CHECK(result);

  std::unique_ptr<base::Value> recommended_value;
  // This remove passes ownership to |recommended_value|.
  if (!result->RemoveWithoutPathExpansion(::onc::kRecommended,
                                          &recommended_value)) {
    return true;
  }

  base::ListValue* recommended_list = nullptr;
  recommended_value->GetAsList(&recommended_list);
  DCHECK(recommended_list);  // The types of field values are already verified.

  if (!managed_onc_) {
    error_or_warning_found_ = true;
    LOG(WARNING) << MessageHeader() << "Found the field '"
                 << ::onc::kRecommended
                 << "' in an unmanaged ONC. Removing it.";
    return true;
  }

  std::unique_ptr<base::ListValue> repaired_recommended(new base::ListValue);
  for (const auto& entry : *recommended_list) {
    std::string field_name;
    if (!entry.GetAsString(&field_name)) {
      NOTREACHED();  // The types of field values are already verified.
      continue;
    }

    const OncFieldSignature* field_signature =
        GetFieldSignature(object_signature, field_name);

    bool found_error = false;
    std::string error_cause;
    if (!field_signature) {
      found_error = true;
      error_cause = "unknown";
    } else if (field_signature->value_signature->onc_type ==
               base::Value::Type::DICTIONARY) {
      found_error = true;
      error_cause = "dictionary-typed";
    }

    if (found_error) {
      error_or_warning_found_ = true;
      path_.push_back(::onc::kRecommended);
      std::string message = MessageHeader() + "The " + error_cause +
          " field '" + field_name + "' cannot be recommended.";
      path_.pop_back();
      if (error_on_wrong_recommended_) {
        LOG(ERROR) << message;
        return false;
      }

      LOG(WARNING) << message;
      continue;
    }

    repaired_recommended->AppendString(field_name);
  }

  result->Set(::onc::kRecommended, std::move(repaired_recommended));
  return true;
}

bool Validator::ValidateClientCertFields(bool allow_cert_type_none,
                                         base::DictionaryValue* result) {
  using namespace ::onc::client_cert;
  const char* const kValidCertTypes[] = {kRef, kPattern, kPKCS11Id};
  std::vector<const char*> valid_cert_types(toVector(kValidCertTypes));
  if (allow_cert_type_none)
    valid_cert_types.push_back(kClientCertTypeNone);
  if (FieldExistsAndHasNoValidValue(*result, kClientCertType, valid_cert_types))
    return false;

  std::string cert_type = GetStringFromDict(*result, kClientCertType);
  bool all_required_exist = true;

  if (cert_type == kPattern)
    all_required_exist &= RequireField(*result, kClientCertPattern);
  else if (cert_type == kRef)
    all_required_exist &= RequireField(*result, kClientCertRef);
  else if (cert_type == kPKCS11Id)
    all_required_exist &= RequireField(*result, kClientCertPKCS11Id);

  return !error_on_missing_field_ || all_required_exist;
}

namespace {

std::string JoinStringRange(const std::vector<const char*>& strings,
                            const std::string& separator) {
  std::vector<base::StringPiece> string_vector(strings.begin(), strings.end());
  return base::JoinString(string_vector, separator);
}

}  // namespace

bool Validator::IsValidValue(const std::string& field_value,
                             const std::vector<const char*>& valid_values) {
  for (const char* it : valid_values) {
    if (field_value == it)
      return true;
  }
  error_or_warning_found_ = true;
  const std::string valid_values_str =
      "[" + JoinStringRange(valid_values, ", ") + "]";
  LOG(ERROR) << MessageHeader() << "Found value '" << field_value
             << "', but expected one of the values " << valid_values_str;
  return false;
}

bool Validator::FieldExistsAndHasNoValidValue(
    const base::DictionaryValue& object,
    const std::string& field_name,
    const std::vector<const char*>& valid_values) {
  std::string actual_value;
  if (!object.GetStringWithoutPathExpansion(field_name, &actual_value))
    return false;

  path_.push_back(field_name);
  const bool valid = IsValidValue(actual_value, valid_values);
  path_.pop_back();
  return !valid;
}

bool Validator::FieldExistsAndIsNotInRange(const base::DictionaryValue& object,
                                           const std::string& field_name,
                                           int lower_bound,
                                           int upper_bound) {
  int actual_value;
  if (!object.GetIntegerWithoutPathExpansion(field_name, &actual_value) ||
      (lower_bound <= actual_value && actual_value <= upper_bound)) {
    return false;
  }
  error_or_warning_found_ = true;
  path_.push_back(field_name);
  LOG(ERROR) << MessageHeader() << "Found value '" << actual_value
             << "', but expected a value in the range [" << lower_bound
             << ", " << upper_bound << "] (boundaries inclusive)";
  path_.pop_back();
  return true;
}

bool Validator::FieldExistsAndIsEmpty(const base::DictionaryValue& object,
                                      const std::string& field_name) {
  const base::Value* value = NULL;
  if (!object.GetWithoutPathExpansion(field_name, &value))
    return false;

  std::string str;
  const base::ListValue* list = NULL;
  if (value->GetAsString(&str)) {
    if (!str.empty())
      return false;
  } else if (value->GetAsList(&list)) {
    if (!list->empty())
      return false;
  } else {
    NOTREACHED();
    return false;
  }

  error_or_warning_found_ = true;
  path_.push_back(field_name);
  LOG(ERROR) << MessageHeader() << "Found an empty string, but expected a "
             << "non-empty string.";
  path_.pop_back();
  return true;
}

bool Validator::ListFieldContainsValidValues(
    const base::DictionaryValue& object,
    const std::string& field_name,
    const std::vector<const char*>& valid_values) {
  const base::ListValue* list = NULL;
  if (object.GetListWithoutPathExpansion(field_name, &list)) {
    path_.push_back(field_name);
    for (const auto& entry : *list) {
      std::string value;
      if (!entry.GetAsString(&value)) {
        NOTREACHED();  // The types of field values are already verified.
        continue;
      }
      if (!IsValidValue(value, valid_values)) {
        path_.pop_back();
        return false;
      }
    }
    path_.pop_back();
  }
  return true;
}

bool Validator::ValidateSSIDAndHexSSID(base::DictionaryValue* object) {
  // Check SSID validity.
  std::string ssid_string;
  if (object->GetStringWithoutPathExpansion(::onc::wifi::kSSID, &ssid_string) &&
      (ssid_string.size() <= 0 ||
       ssid_string.size() > kMaximumSSIDLengthInBytes)) {
    error_or_warning_found_ = true;
    const std::string msg =
        MessageHeader() + ::onc::wifi::kSSID + " has an invalid length.";
    // If the HexSSID field is present, ignore errors in SSID because these
    // might be caused by the usage of a non-UTF-8 encoding when the SSID
    // field was automatically added (see FillInHexSSIDField).
    if (!object->HasKey(::onc::wifi::kHexSSID)) {
      LOG(ERROR) << msg;
      return false;
    }
    LOG(WARNING) << msg;
  }

  // Check HexSSID validity.
  std::string hex_ssid_string;
  if (object->GetStringWithoutPathExpansion(::onc::wifi::kHexSSID,
                                            &hex_ssid_string)) {
    std::vector<uint8_t> decoded_ssid;
    if (!base::HexStringToBytes(hex_ssid_string, &decoded_ssid)) {
      LOG(ERROR) << MessageHeader() << "Field " << ::onc::wifi::kHexSSID
                 << " is not a valid hex representation: \"" << hex_ssid_string
                 << "\"";
      error_or_warning_found_ = true;
      return false;
    }
    if (decoded_ssid.size() <= 0 ||
        decoded_ssid.size() > kMaximumSSIDLengthInBytes) {
      LOG(ERROR) << MessageHeader() << ::onc::wifi::kHexSSID
                 << " has an invalid length.";
      error_or_warning_found_ = true;
      return false;
    }

    // If both SSID and HexSSID are set, check whether they are consistent, i.e.
    // HexSSID contains the UTF-8 encoding of SSID. If not, remove the SSID
    // field.
    if (ssid_string.length() > 0) {
      std::string decoded_ssid_string(
          reinterpret_cast<const char*>(&decoded_ssid[0]), decoded_ssid.size());
      if (ssid_string != decoded_ssid_string) {
        LOG(WARNING) << MessageHeader() << "Fields " << ::onc::wifi::kSSID
                     << " and " << ::onc::wifi::kHexSSID
                     << " contain inconsistent values. Removing "
                     << ::onc::wifi::kSSID << ".";
        error_or_warning_found_ = true;
        object->RemoveWithoutPathExpansion(::onc::wifi::kSSID, nullptr);
      }
    }
  }
  return true;
}

bool Validator::RequireField(const base::DictionaryValue& dict,
                             const std::string& field_name) {
  if (dict.HasKey(field_name))
    return true;
  std::string message = MessageHeader() + "The required field '" + field_name +
      "' is missing.";
  if (error_on_missing_field_) {
    error_or_warning_found_ = true;
    LOG(ERROR) << message;
  } else {
    VLOG(1) << message;
  }
  return false;
}

bool Validator::CheckGuidIsUniqueAndAddToSet(const base::DictionaryValue& dict,
                                             const std::string& key_guid,
                                             std::set<std::string> *guids) {
  std::string guid;
  if (dict.GetStringWithoutPathExpansion(key_guid, &guid)) {
    if (guids->count(guid) != 0) {
      error_or_warning_found_ = true;
      LOG(ERROR) << MessageHeader() << "Found a duplicate GUID " << guid << ".";
      return false;
    }
    guids->insert(guid);
  }
  return true;
}

bool Validator::IsGlobalNetworkConfigInUserImport(
    const base::DictionaryValue& onc_object) {
  if (onc_source_ == ::onc::ONC_SOURCE_USER_IMPORT &&
      onc_object.HasKey(::onc::toplevel_config::kGlobalNetworkConfiguration)) {
    error_or_warning_found_ = true;
    LOG(ERROR) << MessageHeader() << "GlobalNetworkConfiguration is prohibited "
               << "in ONC user imports";
    return true;
  }
  return false;
}

bool Validator::ValidateToplevelConfiguration(base::DictionaryValue* result) {
  using namespace ::onc::toplevel_config;

  const char* const kValidTypes[] = {kUnencryptedConfiguration,
                                     kEncryptedConfiguration};
  const std::vector<const char*> valid_types(toVector(kValidTypes));
  if (FieldExistsAndHasNoValidValue(*result, kType, valid_types))
    return false;

  if (IsGlobalNetworkConfigInUserImport(*result))
    return false;

  return true;
}

bool Validator::ValidateNetworkConfiguration(base::DictionaryValue* result) {
  using namespace ::onc::network_config;

  const char* const kValidTypes[] = {
      ::onc::network_type::kEthernet, ::onc::network_type::kVPN,
      ::onc::network_type::kWiFi,     ::onc::network_type::kCellular,
      ::onc::network_type::kWimax,    ::onc::network_type::kTether};
  const std::vector<const char*> valid_types(toVector(kValidTypes));
  const char* const kValidIPConfigTypes[] = {kIPConfigTypeDHCP,
                                             kIPConfigTypeStatic};
  const std::vector<const char*> valid_ipconfig_types(
      toVector(kValidIPConfigTypes));
  if (FieldExistsAndHasNoValidValue(*result, kType, valid_types) ||
      FieldExistsAndHasNoValidValue(*result, kIPAddressConfigType,
                                    valid_ipconfig_types) ||
      FieldExistsAndHasNoValidValue(*result, kNameServersConfigType,
                                    valid_ipconfig_types) ||
      FieldExistsAndIsEmpty(*result, kGUID)) {
    return false;
  }

  if (!CheckGuidIsUniqueAndAddToSet(*result, kGUID, &network_guids_))
    return false;

  bool all_required_exist = RequireField(*result, kGUID);

  bool remove = false;
  result->GetBooleanWithoutPathExpansion(::onc::kRemove, &remove);
  if (!remove) {
    all_required_exist &=
        RequireField(*result, kName) && RequireField(*result, kType);

    std::string ip_address_config_type =
        GetStringFromDict(*result, kIPAddressConfigType);
    std::string name_servers_config_type =
        GetStringFromDict(*result, kNameServersConfigType);
    if (ip_address_config_type == kIPConfigTypeStatic ||
        name_servers_config_type == kIPConfigTypeStatic) {
      // TODO(pneubeck): Add ValidateStaticIPConfig and confirm that the
      // correct properties are provided based on the config type.
      all_required_exist &= RequireField(*result, kStaticIPConfig);
    }

    std::string type = GetStringFromDict(*result, kType);

    // Prohibit anything but WiFi and Ethernet for device-level policy (which
    // corresponds to shared networks). See also http://crosbug.com/28741.
    if (onc_source_ == ::onc::ONC_SOURCE_DEVICE_POLICY && !type.empty() &&
        type != ::onc::network_type::kWiFi &&
        type != ::onc::network_type::kEthernet) {
      error_or_warning_found_ = true;
      LOG(ERROR) << MessageHeader() << "Networks of type '"
                 << type << "' are prohibited in ONC device policies.";
      return false;
    }

    if (type == ::onc::network_type::kWiFi) {
      all_required_exist &= RequireField(*result, ::onc::network_config::kWiFi);
    } else if (type == ::onc::network_type::kEthernet) {
      all_required_exist &=
          RequireField(*result, ::onc::network_config::kEthernet);
    } else if (type == ::onc::network_type::kCellular) {
      all_required_exist &=
          RequireField(*result, ::onc::network_config::kCellular);
    } else if (type == ::onc::network_type::kWimax) {
      all_required_exist &=
          RequireField(*result, ::onc::network_config::kWimax);
    } else if (type == ::onc::network_type::kVPN) {
      all_required_exist &= RequireField(*result, ::onc::network_config::kVPN);
    } else if (type == ::onc::network_type::kTether) {
      all_required_exist &=
          RequireField(*result, ::onc::network_config::kTether);
    }
  }

  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateEthernet(base::DictionaryValue* result) {
  using namespace ::onc::ethernet;

  const char* const kValidAuthentications[] = {kAuthenticationNone, k8021X};
  const std::vector<const char*> valid_authentications(
      toVector(kValidAuthentications));
  if (FieldExistsAndHasNoValidValue(
          *result, kAuthentication, valid_authentications)) {
    return false;
  }

  bool all_required_exist = true;
  std::string auth = GetStringFromDict(*result, kAuthentication);
  if (auth == k8021X)
    all_required_exist &= RequireField(*result, kEAP);

  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateIPConfig(base::DictionaryValue* result) {
  using namespace ::onc::ipconfig;

  const char* const kValidTypes[] = {kIPv4, kIPv6};
  const std::vector<const char*> valid_types(toVector(kValidTypes));
  if (FieldExistsAndHasNoValidValue(
          *result, ::onc::ipconfig::kType, valid_types))
    return false;

  std::string type = GetStringFromDict(*result, ::onc::ipconfig::kType);
  int lower_bound = 1;
  // In case of missing type, choose higher upper_bound.
  int upper_bound = (type == kIPv4) ? 32 : 128;
  if (FieldExistsAndIsNotInRange(
          *result, kRoutingPrefix, lower_bound, upper_bound)) {
    return false;
  }

  bool all_required_exist = RequireField(*result, kIPAddress) &&
                            RequireField(*result, ::onc::ipconfig::kType);
  if (result->HasKey(kIPAddress))
    all_required_exist &= RequireField(*result, kRoutingPrefix);


  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateWiFi(base::DictionaryValue* result) {
  using namespace ::onc::wifi;

  const char* const kValidSecurities[] = {kSecurityNone, kWEP_PSK, kWEP_8021X,
                                          kWPA_PSK, kWPA_EAP};
  const std::vector<const char*> valid_securities(toVector(kValidSecurities));
  if (FieldExistsAndHasNoValidValue(*result, kSecurity, valid_securities))
    return false;

  if (!ValidateSSIDAndHexSSID(result))
    return false;

  bool all_required_exist = RequireField(*result, kSecurity);

  // One of {kSSID, kHexSSID} must be present.
  if (!result->HasKey(kSSID))
    all_required_exist &= RequireField(*result, kHexSSID);
  if (!result->HasKey(kHexSSID))
    all_required_exist &= RequireField(*result, kSSID);

  std::string security = GetStringFromDict(*result, kSecurity);
  if (security == kWEP_8021X || security == kWPA_EAP)
    all_required_exist &= RequireField(*result, kEAP);
  else if (security == kWEP_PSK || security == kWPA_PSK)
    all_required_exist &= RequireField(*result, kPassphrase);

  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateVPN(base::DictionaryValue* result) {
  using namespace ::onc::vpn;

  const char* const kValidTypes[] = {kIPsec, kTypeL2TP_IPsec, kOpenVPN,
                                     kThirdPartyVpn, kArcVpn};
  const std::vector<const char*> valid_types(toVector(kValidTypes));
  if (FieldExistsAndHasNoValidValue(*result, ::onc::vpn::kType, valid_types))
    return false;

  bool all_required_exist = RequireField(*result, ::onc::vpn::kType);
  std::string type = GetStringFromDict(*result, ::onc::vpn::kType);
  if (type == kOpenVPN) {
    all_required_exist &= RequireField(*result, kOpenVPN);
  } else if (type == kIPsec) {
    all_required_exist &= RequireField(*result, kIPsec);
  } else if (type == kTypeL2TP_IPsec) {
    all_required_exist &=
        RequireField(*result, kIPsec) && RequireField(*result, kL2TP);
  } else if (type == kThirdPartyVpn) {
    all_required_exist &= RequireField(*result, kThirdPartyVpn);
  } else if (type == kArcVpn) {
    all_required_exist &= RequireField(*result, kArcVpn);
  }

  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateIPsec(base::DictionaryValue* result) {
  using namespace ::onc::ipsec;

  const char* const kValidAuthentications[] = {kPSK, kCert};
  const std::vector<const char*> valid_authentications(
      toVector(kValidAuthentications));
  if (FieldExistsAndHasNoValidValue(
          *result, kAuthenticationType, valid_authentications) ||
      FieldExistsAndIsEmpty(*result, kServerCARefs)) {
    return false;
  }

  if (result->HasKey(kServerCARefs) && result->HasKey(kServerCARef)) {
    error_or_warning_found_ = true;
    LOG(ERROR) << MessageHeader() << "At most one of " << kServerCARefs
               << " and " << kServerCARef << " can be set.";
    return false;
  }

  if (!ValidateClientCertFields(false,  // don't allow ClientCertType None
                                result)) {
    return false;
  }

  bool all_required_exist = RequireField(*result, kAuthenticationType) &&
                            RequireField(*result, kIKEVersion);
  std::string auth = GetStringFromDict(*result, kAuthenticationType);
  bool has_server_ca_cert =
      result->HasKey(kServerCARefs) || result->HasKey(kServerCARef);
  if (auth == kCert) {
    all_required_exist &=
        RequireField(*result, ::onc::client_cert::kClientCertType);
    if (!has_server_ca_cert) {
      all_required_exist = false;
      error_or_warning_found_ = true;
      std::string message = MessageHeader() + "The required field '" +
                            kServerCARefs + "' is missing.";
      if (error_on_missing_field_)
        LOG(ERROR) << message;
      else
        LOG(WARNING) << message;
    }
  } else if (has_server_ca_cert) {
    error_or_warning_found_ = true;
    LOG(ERROR) << MessageHeader() << kServerCARefs << " (or " << kServerCARef
               << ") can only be set if " << kAuthenticationType
               << " is set to " << kCert << ".";
    return false;
  }

  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateOpenVPN(base::DictionaryValue* result) {
  using namespace ::onc::openvpn;

  const char* const kValidAuthRetryValues[] = {::onc::openvpn::kNone, kInteract,
                                               kNoInteract};
  const std::vector<const char*> valid_auth_retry_values(
      toVector(kValidAuthRetryValues));
  const char* const kValidCertTlsValues[] = {::onc::openvpn::kNone,
                                             ::onc::openvpn::kServer};
  const std::vector<const char*> valid_cert_tls_values(
      toVector(kValidCertTlsValues));
  const char* const kValidUserAuthTypes[] = {
      ::onc::openvpn_user_auth_type::kNone,
      ::onc::openvpn_user_auth_type::kOTP,
      ::onc::openvpn_user_auth_type::kPassword,
      ::onc::openvpn_user_auth_type::kPasswordAndOTP};
  const std::vector<const char*> valid_user_auth_types(
      toVector(kValidUserAuthTypes));

  if (FieldExistsAndHasNoValidValue(
          *result, kAuthRetry, valid_auth_retry_values) ||
      FieldExistsAndHasNoValidValue(
          *result, kRemoteCertTLS, valid_cert_tls_values) ||
      FieldExistsAndHasNoValidValue(
          *result, kUserAuthenticationType, valid_user_auth_types) ||
      FieldExistsAndIsEmpty(*result, kServerCARefs)) {
    return false;
  }

  // ONC policy prevents the UI from setting properties that are not explicitly
  // listed as 'recommended' (i.e. the default is 'enforced'). Historically
  // the configuration UI ignored this restriction. In order to support legacy
  // ONC configurations, add recommended entries for user authentication
  // properties where appropriate.
  if ((onc_source_ == ::onc::ONC_SOURCE_DEVICE_POLICY ||
       onc_source_ == ::onc::ONC_SOURCE_USER_POLICY)) {
    base::Value* recommended =
        result->FindKeyOfType(::onc::kRecommended, base::Value::Type::LIST);
    if (!recommended)
      recommended = result->SetKey(::onc::kRecommended, base::ListValue());

    // If kUserAuthenticationType is unspecified, allow Password and OTP.
    base::Value::ListStorage& recommended_list = recommended->GetList();
    if (!result->FindKeyOfType(::onc::openvpn::kUserAuthenticationType,
                               base::Value::Type::STRING)) {
      AddKeyToList(::onc::openvpn::kPassword, recommended_list);
      AddKeyToList(::onc::openvpn::kOTP, recommended_list);
    }

    // If client cert type is not provided, empty, or 'None', allow client cert
    // properties.
    std::string client_cert_type =
        GetStringFromDict(*result, ::onc::client_cert::kClientCertType);
    if (client_cert_type.empty() ||
        client_cert_type == ::onc::client_cert::kClientCertTypeNone) {
      AddKeyToList(::onc::client_cert::kClientCertType, recommended_list);
      AddKeyToList(::onc::client_cert::kClientCertPKCS11Id, recommended_list);
    }
  }

  if (result->HasKey(kServerCARefs) && result->HasKey(kServerCARef)) {
    error_or_warning_found_ = true;
    LOG(ERROR) << MessageHeader() << "At most one of " << kServerCARefs
               << " and " << kServerCARef << " can be set.";
    return false;
  }

  if (!ValidateClientCertFields(true /* allow ClientCertType None */, result))
    return false;

  bool all_required_exist =
      RequireField(*result, ::onc::client_cert::kClientCertType);

  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateThirdPartyVPN(base::DictionaryValue* result) {
  const bool all_required_exist =
      RequireField(*result, ::onc::third_party_vpn::kExtensionID);

  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateARCVPN(base::DictionaryValue* result) {
  const bool all_required_exist =
      RequireField(*result, ::onc::arc_vpn::kTunnelChrome);

  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateVerifyX509(base::DictionaryValue* result) {
  using namespace ::onc::verify_x509;

  const char* const kValidTypes[] = {types::kName, types::kNamePrefix,
                                     types::kSubject};
  const std::vector<const char*> valid_types(toVector(kValidTypes));

  if (FieldExistsAndHasNoValidValue(*result, kType, valid_types))
    return false;

  bool all_required_exist = RequireField(*result, kName);

  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateCertificatePattern(base::DictionaryValue* result) {
  using namespace ::onc::client_cert;

  bool all_required_exist = true;
  if (!result->HasKey(kSubject) && !result->HasKey(kIssuer) &&
      !result->HasKey(kIssuerCARef)) {
    error_or_warning_found_ = true;
    all_required_exist = false;
    std::string message = MessageHeader() + "None of the fields '" + kSubject +
        "', '" + kIssuer + "', and '" + kIssuerCARef +
        "' is present, but at least one is required.";
    if (error_on_missing_field_)
      LOG(ERROR) << message;
    else
      LOG(WARNING) << message;
  }

  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateGlobalNetworkConfiguration(
    base::DictionaryValue* result) {
  using namespace ::onc::global_network_config;
  using namespace ::onc::network_config;

  // Validate kDisableNetworkTypes field.
  const base::ListValue* disabled_network_types = NULL;
  if (result->GetListWithoutPathExpansion(kDisableNetworkTypes,
                                          &disabled_network_types)) {
    // The kDisableNetworkTypes field is only allowed in device policy.
    if (!disabled_network_types->empty() &&
        onc_source_ != ::onc::ONC_SOURCE_DEVICE_POLICY) {
      error_or_warning_found_ = true;
      LOG(ERROR) << "Disabled network types only allowed in device policy.";
      return false;
    }
  }

  if (result->HasKey(kAllowOnlyPolicyNetworksToConnect)) {
    // The kAllowOnlyPolicyNetworksToConnect field is only allowed in device
    // policy.
    if (onc_source_ != ::onc::ONC_SOURCE_DEVICE_POLICY) {
      error_or_warning_found_ = true;
      LOG(ERROR)
          << "AllowOnlyPolicyNetworksToConnect only allowed in device policy.";
      return false;
    }
  }

  // Ensure the list contains only legitimate network type identifiers.
  const char* const kValidNetworkTypeValues[] = {kCellular, kEthernet, kWiFi,
                                                 kWimax, kTether};
  const std::vector<const char*> valid_network_type_values(
      toVector(kValidNetworkTypeValues));
  if (!ListFieldContainsValidValues(*result, kDisableNetworkTypes,
                                    valid_network_type_values)) {
    return false;
  }
  return true;
}

bool Validator::ValidateProxySettings(base::DictionaryValue* result) {
  using namespace ::onc::proxy;

  const char* const kValidTypes[] = {kDirect, kManual, kPAC, kWPAD};
  const std::vector<const char*> valid_types(toVector(kValidTypes));
  if (FieldExistsAndHasNoValidValue(*result, ::onc::proxy::kType, valid_types))
    return false;

  bool all_required_exist = RequireField(*result, ::onc::proxy::kType);
  std::string type = GetStringFromDict(*result, ::onc::proxy::kType);
  if (type == kManual)
    all_required_exist &= RequireField(*result, kManual);
  else if (type == kPAC)
    all_required_exist &= RequireField(*result, kPAC);

  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateProxyLocation(base::DictionaryValue* result) {
  using namespace ::onc::proxy;

  bool all_required_exist =
      RequireField(*result, kHost) && RequireField(*result, kPort);

  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateEAP(base::DictionaryValue* result) {
  using namespace ::onc::eap;

  const char* const kValidInnerValues[] = {
      kAutomatic, kGTC, kMD5, kMSCHAPv2, kPAP};
  const std::vector<const char*> valid_inner_values(
      toVector(kValidInnerValues));
  const char* const kValidOuterValues[] = {
      kPEAP, kEAP_TLS, kEAP_TTLS, kLEAP, kEAP_SIM, kEAP_FAST, kEAP_AKA};
  const std::vector<const char*> valid_outer_values(
      toVector(kValidOuterValues));

  if (FieldExistsAndHasNoValidValue(*result, kInner, valid_inner_values) ||
      FieldExistsAndHasNoValidValue(*result, kOuter, valid_outer_values) ||
      FieldExistsAndIsEmpty(*result, kServerCARefs)) {
    return false;
  }

  if (result->HasKey(kServerCARefs) && result->HasKey(kServerCARef)) {
    error_or_warning_found_ = true;
    LOG(ERROR) << MessageHeader() << "At most one of " << kServerCARefs
               << " and " << kServerCARef << " can be set.";
    return false;
  }

  if (!ValidateClientCertFields(true /* allow ClientCertType None */, result))
    return false;

  bool all_required_exist = RequireField(*result, kOuter);

  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateCertificate(base::DictionaryValue* result) {
  using namespace ::onc::certificate;

  const char* const kValidTypes[] = {kClient, kServer, kAuthority};
  const std::vector<const char*> valid_types(toVector(kValidTypes));
  if (FieldExistsAndHasNoValidValue(*result, kType, valid_types) ||
      FieldExistsAndIsEmpty(*result, kGUID)) {
    return false;
  }

  std::string type = GetStringFromDict(*result, kType);

  if (!CheckGuidIsUniqueAndAddToSet(*result, kGUID, &certificate_guids_))
    return false;

  bool all_required_exist = RequireField(*result, kGUID);

  bool remove = false;
  result->GetBooleanWithoutPathExpansion(::onc::kRemove, &remove);
  if (remove) {
    error_or_warning_found_ = true;
    LOG(ERROR) << MessageHeader()
               << "Removal of certificates is not supported.";
    return false;
  }

  all_required_exist &= RequireField(*result, kType);

  if (type == kClient)
    all_required_exist &= RequireField(*result, kPKCS12);
  else if (type == kServer || type == kAuthority)
    all_required_exist &= RequireField(*result, kX509);

  return !error_on_missing_field_ || all_required_exist;
}

bool Validator::ValidateTether(base::DictionaryValue* result) {
  using namespace ::onc::tether;

  int battery_percentage;
  if (!result->GetIntegerWithoutPathExpansion(kBatteryPercentage,
                                              &battery_percentage) ||
      battery_percentage < 0 || battery_percentage > 100) {
    // Battery percentage must be present and within [0, 100].
    error_or_warning_found_ = true;
    return false;
  }

  int signal_strength;
  if (!result->GetIntegerWithoutPathExpansion(kSignalStrength,
                                              &signal_strength) ||
      signal_strength < 0 || signal_strength > 100) {
    // Signal strength must be present and within [0, 100].
    error_or_warning_found_ = true;
    return false;
  }

  std::string carrier = GetStringFromDict(*result, kCarrier);
  if (carrier.empty()) {
    // Carrier must be a non-empty string.
    error_or_warning_found_ = true;
    return false;
  }

  bool all_required_exist = RequireField(*result, kHasConnectedToHost);
  if (!all_required_exist) {
    error_or_warning_found_ = true;
  }

  return !error_on_missing_field_ || all_required_exist;
}

std::string Validator::MessageHeader() {
  std::string path = path_.empty() ? "toplevel" : base::JoinString(path_, ".");
  std::string message = "At " + path + ": ";
  return message;
}

}  // namespace onc
}  // namespace chromeos
