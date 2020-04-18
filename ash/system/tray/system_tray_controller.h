// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_SYSTEM_TRAY_CONTROLLER_H_
#define ASH_SYSTEM_TRAY_SYSTEM_TRAY_CONTROLLER_H_

#include "ash/ash_export.h"
#include "ash/public/interfaces/system_tray.mojom.h"
#include "base/compiler_specific.h"
#include "base/i18n/time_formatting.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace ash {

// Both implements mojom::SystemTray and wraps the mojom::SystemTrayClient
// interface. Implements both because it caches state pushed down from the
// browser process via SystemTray so it can be synchronously queried inside ash.
// Lives on the main thread.
class ASH_EXPORT SystemTrayController : public mojom::SystemTray {
 public:
  SystemTrayController();
  ~SystemTrayController() override;

  // Wrappers around the mojom::SystemTrayClient interface.
  void ShowSettings();
  void ShowBluetoothSettings();
  void ShowBluetoothPairingDialog(const std::string& address,
                                  const base::string16& name_for_display,
                                  bool paired,
                                  bool connected);
  void ShowDateSettings();
  void ShowSetTimeDialog();
  void ShowDisplaySettings();
  void ShowPowerSettings();
  void ShowChromeSlow();
  void ShowIMESettings();
  void ShowAboutChromeOS();
  void ShowHelp();
  void ShowAccessibilityHelp();
  void ShowAccessibilitySettings();
  void ShowPaletteHelp();
  void ShowPaletteSettings();
  void ShowPublicAccountInfo();
  void ShowEnterpriseInfo();
  void ShowNetworkConfigure(const std::string& network_id);
  void ShowNetworkCreate(const std::string& type);
  void ShowThirdPartyVpnCreate(const std::string& extension_id);
  void ShowArcVpnCreate(const std::string& app_id);
  void ShowNetworkSettings(const std::string& network_id);
  void ShowMultiDeviceSetup();
  void RequestRestartForUpdate();

  // Binds the mojom::SystemTray interface to this object.
  void BindRequest(mojom::SystemTrayRequest request);

  // mojom::SystemTray overrides. Public for testing.
  void SetClient(mojom::SystemTrayClientPtr client) override;
  void SetPrimaryTrayEnabled(bool enabled) override;
  void SetPrimaryTrayVisible(bool visible) override;
  void SetUse24HourClock(bool use_24_hour) override;
  void SetEnterpriseDisplayDomain(const std::string& enterprise_display_domain,
                                  bool active_directory_managed) override;
  void SetPerformanceTracingIconVisible(bool visible) override;
  void ShowUpdateIcon(mojom::UpdateSeverity severity,
                      bool factory_reset_required,
                      mojom::UpdateType update_type) override;
  void SetUpdateOverCellularAvailableIconVisible(bool visible) override;

 private:
  // Client interface in chrome browser. May be null in tests.
  mojom::SystemTrayClientPtr system_tray_client_;

  // Bindings for users of the mojo interface.
  mojo::BindingSet<mojom::SystemTray> bindings_;

  DISALLOW_COPY_AND_ASSIGN(SystemTrayController);
};

}  // namspace ash

#endif  // ASH_SYSTEM_TRAY_SYSTEM_TRAY_CONTROLLER_H_
