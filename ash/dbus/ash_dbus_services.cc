// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/dbus/ash_dbus_services.h"

#include "ash/dbus/display_service_provider.h"
#include "ash/dbus/url_handler_service_provider.h"
#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/services/cros_dbus_service.h"
#include "chromeos/dbus/session_manager_client.h"
#include "dbus/object_path.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace ash {

AshDBusServices::AshDBusServices() {
  // TODO(stevenjb): Figure out where else the D-Bus thread is getting
  // initialized and then always init it here when we have the MASH
  // config after the contention is sorted out.
  if (!chromeos::DBusThreadManager::IsInitialized()) {
    chromeos::DBusThreadManager::Initialize(
        chromeos::DBusThreadManager::kShared);
    initialized_dbus_thread_ = true;
  }

  url_handler_service_ = chromeos::CrosDBusService::Create(
      chromeos::kUrlHandlerServiceName,
      dbus::ObjectPath(chromeos::kUrlHandlerServicePath),
      chromeos::CrosDBusService::CreateServiceProviderList(
          std::make_unique<UrlHandlerServiceProvider>()));

  display_service_ = chromeos::CrosDBusService::Create(
      chromeos::kDisplayServiceName,
      dbus::ObjectPath(chromeos::kDisplayServicePath),
      chromeos::CrosDBusService::CreateServiceProviderList(
          std::make_unique<DisplayServiceProvider>()));
}

void AshDBusServices::EmitAshInitialized() {
  chromeos::DBusThreadManager::Get()
      ->GetSessionManagerClient()
      ->EmitAshInitialized();
}

AshDBusServices::~AshDBusServices() {
  display_service_.reset();
  url_handler_service_.reset();
  if (initialized_dbus_thread_) {
    chromeos::DBusThreadManager::Shutdown();
  }
}

}  // namespace ash
