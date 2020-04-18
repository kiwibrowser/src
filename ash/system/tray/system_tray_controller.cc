// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray/system_tray_controller.h"

#include "ash/public/cpp/ash_features.h"
#include "ash/root_window_controller.h"
#include "ash/shell.h"
#include "ash/system/model/clock_model.h"
#include "ash/system/model/enterprise_domain_model.h"
#include "ash/system/model/system_tray_model.h"
#include "ash/system/model/tracing_model.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/system/unified/unified_system_tray.h"
#include "ash/system/update/tray_update.h"

namespace ash {

SystemTrayController::SystemTrayController() {}

SystemTrayController::~SystemTrayController() = default;

void SystemTrayController::ShowSettings() {
  if (system_tray_client_)
    system_tray_client_->ShowSettings();
}

void SystemTrayController::ShowBluetoothSettings() {
  if (system_tray_client_)
    system_tray_client_->ShowBluetoothSettings();
}

void SystemTrayController::ShowBluetoothPairingDialog(
    const std::string& address,
    const base::string16& name_for_display,
    bool paired,
    bool connected) {
  if (system_tray_client_) {
    system_tray_client_->ShowBluetoothPairingDialog(address, name_for_display,
                                                    paired, connected);
  }
}

void SystemTrayController::ShowDateSettings() {
  if (system_tray_client_)
    system_tray_client_->ShowDateSettings();
}

void SystemTrayController::ShowSetTimeDialog() {
  if (system_tray_client_)
    system_tray_client_->ShowSetTimeDialog();
}

void SystemTrayController::ShowDisplaySettings() {
  if (system_tray_client_)
    system_tray_client_->ShowDisplaySettings();
}

void SystemTrayController::ShowPowerSettings() {
  if (system_tray_client_)
    system_tray_client_->ShowPowerSettings();
}

void SystemTrayController::ShowChromeSlow() {
  if (system_tray_client_)
    system_tray_client_->ShowChromeSlow();
}

void SystemTrayController::ShowIMESettings() {
  if (system_tray_client_)
    system_tray_client_->ShowIMESettings();
}

void SystemTrayController::ShowAboutChromeOS() {
  if (system_tray_client_)
    system_tray_client_->ShowAboutChromeOS();
}

void SystemTrayController::ShowHelp() {
  if (system_tray_client_)
    system_tray_client_->ShowHelp();
}

void SystemTrayController::ShowAccessibilityHelp() {
  if (system_tray_client_)
    system_tray_client_->ShowAccessibilityHelp();
}

void SystemTrayController::ShowAccessibilitySettings() {
  if (system_tray_client_)
    system_tray_client_->ShowAccessibilitySettings();
}

void SystemTrayController::ShowPaletteHelp() {
  if (system_tray_client_)
    system_tray_client_->ShowPaletteHelp();
}

void SystemTrayController::ShowPaletteSettings() {
  if (system_tray_client_)
    system_tray_client_->ShowPaletteSettings();
}

void SystemTrayController::ShowPublicAccountInfo() {
  if (system_tray_client_)
    system_tray_client_->ShowPublicAccountInfo();
}

void SystemTrayController::ShowEnterpriseInfo() {
  if (system_tray_client_)
    system_tray_client_->ShowEnterpriseInfo();
}

void SystemTrayController::ShowNetworkConfigure(const std::string& network_id) {
  if (system_tray_client_)
    system_tray_client_->ShowNetworkConfigure(network_id);
}

void SystemTrayController::ShowNetworkCreate(const std::string& type) {
  if (system_tray_client_)
    system_tray_client_->ShowNetworkCreate(type);
}

void SystemTrayController::ShowThirdPartyVpnCreate(
    const std::string& extension_id) {
  if (system_tray_client_)
    system_tray_client_->ShowThirdPartyVpnCreate(extension_id);
}

void SystemTrayController::ShowArcVpnCreate(const std::string& app_id) {
  if (system_tray_client_)
    system_tray_client_->ShowArcVpnCreate(app_id);
}

void SystemTrayController::ShowNetworkSettings(const std::string& network_id) {
  if (system_tray_client_)
    system_tray_client_->ShowNetworkSettings(network_id);
}

void SystemTrayController::ShowMultiDeviceSetup() {
  if (system_tray_client_)
    system_tray_client_->ShowMultiDeviceSetup();
}

void SystemTrayController::RequestRestartForUpdate() {
  if (system_tray_client_)
    system_tray_client_->RequestRestartForUpdate();
}

void SystemTrayController::BindRequest(mojom::SystemTrayRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void SystemTrayController::SetClient(mojom::SystemTrayClientPtr client) {
  system_tray_client_ = std::move(client);
}

void SystemTrayController::SetPrimaryTrayEnabled(bool enabled) {
  // We disable the UI to prevent user from interacting with UI elements,
  // particularly with the system tray menu. However, in case if the system tray
  // bubble is opened at this point, it remains opened and interactive even
  // after SystemTray::SetEnabled(false) call, which can be dangerous
  // (http://crbug.com/497080). Close the menu to fix it. Calling
  // SystemTray::SetEnabled(false) guarantees, that the menu will not be opened
  // until the UI is enabled again.

  if (features::IsSystemTrayUnifiedEnabled()) {
    UnifiedSystemTray* tray = Shell::GetPrimaryRootWindowController()
                                  ->GetStatusAreaWidget()
                                  ->unified_system_tray();
    if (!tray)
      return;

    if (!enabled && tray->IsBubbleShown())
      tray->CloseBubble();

    tray->SetEnabled(enabled);
  } else {
    ash::SystemTray* tray =
        Shell::GetPrimaryRootWindowController()->GetSystemTray();
    if (!tray)
      return;

    if (!enabled && tray->HasSystemBubble())
      tray->CloseBubble();

    tray->SetEnabled(enabled);
  }
}

void SystemTrayController::SetPrimaryTrayVisible(bool visible) {
  auto* status_area =
      Shell::GetPrimaryRootWindowController()->GetStatusAreaWidget();
  if (status_area)
    status_area->SetSystemTrayVisibility(visible);
}

void SystemTrayController::SetUse24HourClock(bool use_24_hour) {
  Shell::Get()->system_tray_model()->SetUse24HourClock(use_24_hour);
}

void SystemTrayController::SetEnterpriseDisplayDomain(
    const std::string& enterprise_display_domain,
    bool active_directory_managed) {
  Shell::Get()
      ->system_tray_model()
      ->SetEnterpriseDisplayDomain(enterprise_display_domain,
                                   active_directory_managed);
}

void SystemTrayController::SetPerformanceTracingIconVisible(bool visible) {
  Shell::Get()->system_tray_model()->SetPerformanceTracingIconVisible(visible);
}

void SystemTrayController::ShowUpdateIcon(mojom::UpdateSeverity severity,
                                          bool factory_reset_required,
                                          mojom::UpdateType update_type) {
  Shell::Get()->system_tray_model()->ShowUpdateIcon(
      severity, factory_reset_required, update_type);
}

void SystemTrayController::SetUpdateOverCellularAvailableIconVisible(
    bool visible) {
  Shell::Get()->system_tray_model()->SetUpdateOverCellularAvailableIconVisible(
      visible);
}

}  // namespace ash
