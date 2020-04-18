// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_FAKE_LORGNETTE_MANAGER_CLIENT_H_
#define CHROMEOS_DBUS_FAKE_LORGNETTE_MANAGER_CLIENT_H_

#include <map>
#include <string>
#include <tuple>

#include "base/macros.h"
#include "chromeos/dbus/lorgnette_manager_client.h"

namespace chromeos {

// Lorgnette LorgnetteManagerClient implementation used on Linux desktop,
// which does nothing.
class CHROMEOS_EXPORT FakeLorgnetteManagerClient
    : public LorgnetteManagerClient {
 public:
  FakeLorgnetteManagerClient();
  ~FakeLorgnetteManagerClient() override;

  void Init(dbus::Bus* bus) override;

  void ListScanners(DBusMethodCallback<ScannerTable> callback) override;
  void ScanImageToString(std::string device_name,
                         const ScanProperties& properties,
                         DBusMethodCallback<std::string> callback) override;

  // Adds a fake scanner table entry, which will be returned by ListScanners().
  void AddScannerTableEntry(const std::string& device_name,
                            const ScannerTableEntry& entry);

  // Adds a fake scan data, which will be returned by ScanImageToString(),
  // if |device_name| and |properties| are matched.
  void AddScanData(const std::string& device_name,
                   const ScanProperties& properties,
                   const std::string& data);

 private:
  ScannerTable scanner_table_;

  // Use tuple for a map below, which has pre-defined "less", for convenience.
  using ScanDataKey = std::tuple<std::string /* device_name */,
                                 std::string /* ScanProperties.mode */,
                                 int /* Scanproperties.resolution_dpi */>;
  std::map<ScanDataKey, std::string /* data */> scan_data_;

  DISALLOW_COPY_AND_ASSIGN(FakeLorgnetteManagerClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_FAKE_LORGNETTE_MANAGER_CLIENT_H_
