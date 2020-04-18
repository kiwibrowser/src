// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GEOLOCATION_WIFI_DATA_PROVIDER_COMMON_WIN_H_
#define DEVICE_GEOLOCATION_WIFI_DATA_PROVIDER_COMMON_WIN_H_

#include <windows.h>
#include <ntddndis.h>

#include "device/geolocation/wifi_data_provider.h"

namespace device {

// Extracts access point data from the NDIS_802_11_BSSID_LIST structure and
// appends it to the data vector. Returns the number of access points for which
// data was extracted.
int GetDataFromBssIdList(const NDIS_802_11_BSSID_LIST& bss_id_list,
                         int list_size,
                         WifiData::AccessPointDataSet* data);

}  // namespace device

#endif  // DEVICE_GEOLOCATION_WIFI_DATA_PROVIDER_COMMON_WIN_H_
