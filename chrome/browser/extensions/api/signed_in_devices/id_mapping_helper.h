// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SIGNED_IN_DEVICES_ID_MAPPING_HELPER_H__
#define CHROME_BROWSER_EXTENSIONS_API_SIGNED_IN_DEVICES_ID_MAPPING_HELPER_H__

#include <memory>
#include <string>
#include <vector>


namespace base {
class DictionaryValue;
}  // namespace base

namespace syncer {
class DeviceInfo;
}  // namespace syncer

class Profile;

namespace extensions {

// In order to not expose unique GUIDs for devices to third pary apps,
// the unique GUIDs are mapped to local ids and local ids are exposed to apps.
// The functions in this file are helper routines to do the mapping.

// Gets public id from GUID, given a dictionary that has the mapping.
// If it cannot find the GUID the public id returned will be empty.
std::string GetPublicIdFromGUID(
    const base::DictionaryValue& id_mapping,
    const std::string& guid);

// Gets the GUID from public id given a dictionary that has the mapping.
// If it cannot find the public id, the GUID returned will be empty.
std::string GetGUIDFromPublicId(
    const base::DictionaryValue& id_mapping,
    const std::string& id);

// Creates public id for devices that don't have a public id. To create mappings
// from scratch an empty dictionary must be passed. The dictionary will be
// updated with the mappings. The |device_info| objects will also be updated
// with the public ids.
// The dictionary would have the public id as the key and the
// device guid as the value.
void CreateMappingForUnmappedDevices(
    const std::vector<std::unique_ptr<syncer::DeviceInfo>>& device_info,
    base::DictionaryValue* value);

// Gets the device info for a given client id. If the device is not found
// the returned pointer would be null.
std::unique_ptr<syncer::DeviceInfo> GetDeviceInfoForClientId(
    const std::string& client_id,
    const std::string& extension_id,
    Profile* profile);

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SIGNED_IN_DEVICES_ID_MAPPING_HELPER_H__
