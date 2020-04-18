// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_LORGNETTE_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_LORGNETTE_DBUS_CONSTANTS_H_

namespace lorgnette {
const char kManagerServiceName[] = "org.chromium.lorgnette";
const char kManagerServiceInterface[] = "org.chromium.lorgnette.Manager";
const char kManagerServicePath[] = "/org/chromium/lorgnette/Manager";
const char kManagerServiceError[] = "org.chromium.lorgnette.Error";

// Methods.
const char kListScannersMethod[] = "ListScanners";
const char kScanImageMethod[] = "ScanImage";

// Attributes of scanners returned from "ListScanners".
const char kScannerPropertyManufacturer[] = "Manufacturer";
const char kScannerPropertyModel[] = "Model";
const char kScannerPropertyType[] = "Type";

// Parameters supplied to a "ScanImage" request.
const char kScanPropertyMode[] = "Mode";
const char kScanPropertyModeColor[] = "Color";
const char kScanPropertyModeGray[] = "Gray";
const char kScanPropertyModeLineart[] = "Lineart";
const char kScanPropertyResolution[] = "Resolution";
}  // namespace lorgnette

#endif  // SYSTEM_API_DBUS_LORGNETTE_DBUS_CONSTANTS_H_
