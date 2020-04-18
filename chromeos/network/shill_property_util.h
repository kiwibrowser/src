// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_SHILL_PROPERTY_UTIL_H_
#define CHROMEOS_NETWORK_SHILL_PROPERTY_UTIL_H_

#include <memory>
#include <string>

#include "chromeos/chromeos_export.h"

namespace base {
class DictionaryValue;
class Value;
}

namespace chromeos {

class NetworkUIData;

namespace shill_property_util {

// Sets the |ssid| in |properties|.
CHROMEOS_EXPORT void SetSSID(const std::string& ssid,
                             base::DictionaryValue* properties);

// Returns the SSID from |properties| in UTF-8 encoding. If |verbose_logging| is
// true, detailed DEBUG log events will be added to the device event log. If
// |unknown_encoding| != nullptr, it is set to whether the SSID is of unknown
// encoding.
CHROMEOS_EXPORT std::string GetSSIDFromProperties(
    const base::DictionaryValue& properties,
    bool verbose_logging,
    bool* unknown_encoding);

// Returns the GUID (if available), SSID, or Name from |properties|. Only used
// for logging and debugging.
CHROMEOS_EXPORT std::string GetNetworkIdFromProperties(
    const base::DictionaryValue& properties);

// Returns the name for the network represented by the Shill |properties|. For
// WiFi it refers to the HexSSID.
CHROMEOS_EXPORT std::string GetNameFromProperties(
    const std::string& service_path,
    const base::DictionaryValue& properties);

// Returns the UIData specified by |value|. Returns NULL if the value cannot be
// parsed.
std::unique_ptr<NetworkUIData> GetUIDataFromValue(const base::Value& value);

// Returns the NetworkUIData parsed from the UIData property of
// |shill_dictionary|. If parsing fails or the field doesn't exist, returns
// NULL.
std::unique_ptr<NetworkUIData> GetUIDataFromProperties(
    const base::DictionaryValue& shill_dictionary);

// Sets the UIData property in |shill_dictionary| to the serialization of
// |ui_data|.
void SetUIData(const NetworkUIData& ui_data,
               base::DictionaryValue* shill_dictionary);

// Copy configuration properties required by Shill to identify a network in the
// format that Shill expects on writes.
// Only WiFi, VPN, Ethernet and EthernetEAP are supported. Wimax and Cellular
// are not supported.
// If |properties_read_from_shill| is true, it is assumed that
// |service_properties| has the format that Shill exposes on reads, as opposed
// to property dictionaries which are sent to Shill. Returns true only if all
// required properties could be copied.
bool CopyIdentifyingProperties(const base::DictionaryValue& service_properties,
                               const bool properties_read_from_shill,
                               base::DictionaryValue* dest);

// Compares the identifying configuration properties of |new_properties| and
// |old_properties|, returns true if they are identical. |new_properties| must
// have the form that Shill expects on writes. |old_properties| must have the
// form that Shill exposes on reads. See also CopyIdentifyingProperties. Only
// WiFi, VPN, Ethernet and EthernetEAP are supported. Wimax and Cellular are not
// supported.
bool DoIdentifyingPropertiesMatch(
    const base::DictionaryValue& new_properties,
    const base::DictionaryValue& old_properties);

// Returns false if |key| is something that should not be logged either
// because it is sensitive or noisy. Note: this is not necessarily
// comprehensive, do not use it for anything genuinely sensitive (user logs
// should always be treated as sensitive data, but sometimes they end up
// attached to public issues so this helps prevent accidents, but it should not
// be relied upon).
bool IsLoggableShillProperty(const std::string& key);

}  // namespace shill_property_util

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_SHILL_PROPERTY_UTIL_H_
