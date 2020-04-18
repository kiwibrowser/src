// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/window_manager_service.h"

#include <memory>
#include <utility>

#include "ash/mojo_interface_factory.h"
#include "ash/network_connect_delegate_mus.h"
#include "ash/shell.h"
#include "ash/system/power/power_status.h"
#include "ash/window_manager.h"
#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/audio/cras_audio_handler.h"
#include "chromeos/cryptohome/system_salt_getter.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_policy_controller.h"
#include "chromeos/network/network_connect.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/system/fake_statistics_provider.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/dbus/bluez_dbus_manager.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/ui/common/accelerator_util.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/events/event.h"
#include "ui/views/mus/aura_init.h"

namespace ash {

WindowManagerService::WindowManagerService(bool show_primary_host_on_connect)
    : show_primary_host_on_connect_(show_primary_host_on_connect) {}

WindowManagerService::~WindowManagerService() {
  // Verify that we created a WindowManager before attempting to tear everything
  // down. In some fast running tests OnStart may never have been called.
  if (!window_manager_.get())
    return;

  // Destroy the WindowManager while still valid. This way we ensure
  // OnWillDestroyRootWindowController() is called (if it hasn't been already).
  window_manager_.reset();

  statistics_provider_.reset();
  ShutdownComponents();
}

service_manager::Connector* WindowManagerService::GetConnector() {
  return context() ? context()->connector() : nullptr;
}

void WindowManagerService::InitWindowManager(
    std::unique_ptr<aura::WindowTreeClient> window_tree_client,
    bool init_network_handler) {
  // Tests may have already set the WindowTreeClient.
  if (!aura::Env::GetInstance()->HasWindowTreeClient())
    aura::Env::GetInstance()->SetWindowTreeClient(window_tree_client.get());
  InitializeComponents(init_network_handler);

  // TODO(jamescook): Refactor StatisticsProvider so we can get just the data
  // we need in ash. Right now StatisticsProviderImpl launches the crossystem
  // binary to get system data, which we don't want to do twice on startup.
  statistics_provider_.reset(
      new chromeos::system::ScopedFakeStatisticsProvider());
  statistics_provider_->SetMachineStatistic("initial_locale", "en-US");
  statistics_provider_->SetMachineStatistic("keyboard_layout", "");

  window_manager_->Init(std::move(window_tree_client),
                        /*initial_display_prefs=*/nullptr);
}

void WindowManagerService::InitializeComponents(bool init_network_handler) {
  // Must occur after mojo::ApplicationRunner has initialized AtExitManager, but
  // before WindowManager::Init(). Tests might initialize their own instance.
  if (!chromeos::DBusThreadManager::IsInitialized()) {
    chromeos::DBusThreadManager::Initialize(
        chromeos::DBusThreadManager::kShared);
    dbus_thread_manager_initialized_ = true;
  }
  chromeos::PowerPolicyController::Initialize(
      chromeos::DBusThreadManager::Get()->GetPowerManagerClient());

  // See ChromeBrowserMainPartsChromeos for ordering details.
  bluez::BluezDBusManager::Initialize(
      chromeos::DBusThreadManager::Get()->GetSystemBus(),
      chromeos::DBusThreadManager::Get()->IsUsingFakes());
  if (init_network_handler && !chromeos::NetworkHandler::IsInitialized()) {
    chromeos::NetworkHandler::Initialize();
    network_handler_initialized_ = true;
  }
  network_connect_delegate_.reset(new NetworkConnectDelegateMus());
  chromeos::NetworkConnect::Initialize(network_connect_delegate_.get());
  // TODO(jamescook): Initialize real audio handler.
  chromeos::CrasAudioHandler::InitializeForTesting();
  chromeos::SystemSaltGetter::Initialize();
}

void WindowManagerService::ShutdownComponents() {
  // NOTE: PowerStatus is shutdown by Shell.
  chromeos::SystemSaltGetter::Shutdown();
  chromeos::CrasAudioHandler::Shutdown();
  chromeos::NetworkConnect::Shutdown();
  network_connect_delegate_.reset();
  // We may not have started the NetworkHandler.
  if (network_handler_initialized_)
    chromeos::NetworkHandler::Shutdown();
  device::BluetoothAdapterFactory::Shutdown();
  bluez::BluezDBusManager::Shutdown();
  chromeos::PowerPolicyController::Shutdown();
  if (dbus_thread_manager_initialized_)
    chromeos::DBusThreadManager::Shutdown();
}

void WindowManagerService::OnStart() {
  mojo_interface_factory::RegisterInterfaces(
      &registry_, base::ThreadTaskRunnerHandle::Get());

  const bool register_path_provider = running_standalone_;
  aura_init_ = views::AuraInit::Create(
      context()->connector(), context()->identity(),
      "ash_service_resources.pak", "ash_service_resources_200.pak", nullptr,
      views::AuraInit::Mode::AURA_MUS_WINDOW_MANAGER, register_path_provider);
  if (!aura_init_) {
    context()->QuitNow();
    return;
  }
  window_manager_ = std::make_unique<WindowManager>(
      context()->connector(), show_primary_host_on_connect_);

  const bool automatically_create_display_roots = false;
  std::unique_ptr<aura::WindowTreeClient> window_tree_client =
      aura::WindowTreeClient::CreateForWindowManager(
          context()->connector(), window_manager_.get(), window_manager_.get(),
          automatically_create_display_roots);

  const bool init_network_handler = true;
  InitWindowManager(std::move(window_tree_client), init_network_handler);
}

void WindowManagerService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  // namespace ash
