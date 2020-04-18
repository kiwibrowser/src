// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/dbus_thread_manager.h"

#include <utility>

#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/sys_info.h"
#include "base/threading/thread.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/arc_midis_client.h"
#include "chromeos/dbus/arc_obb_mounter_client.h"
#include "chromeos/dbus/arc_oemcrypto_client.h"
#include "chromeos/dbus/auth_policy_client.h"
#include "chromeos/dbus/biod/biod_client.h"
#include "chromeos/dbus/cec_service_client.h"
#include "chromeos/dbus/concierge_client.h"
#include "chromeos/dbus/cras_audio_client.h"
#include "chromeos/dbus/cros_disks_client.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/dbus_client.h"
#include "chromeos/dbus/dbus_clients_browser.h"
#include "chromeos/dbus/dbus_clients_common.h"
#include "chromeos/dbus/debug_daemon_client.h"
#include "chromeos/dbus/easy_unlock_client.h"
#include "chromeos/dbus/gsm_sms_client.h"
#include "chromeos/dbus/hammerd_client.h"
#include "chromeos/dbus/image_burner_client.h"
#include "chromeos/dbus/image_loader_client.h"
#include "chromeos/dbus/lorgnette_manager_client.h"
#include "chromeos/dbus/media_analytics_client.h"
#include "chromeos/dbus/modem_messaging_client.h"
#include "chromeos/dbus/permission_broker_client.h"
#include "chromeos/dbus/power_manager_client.h"
#include "chromeos/dbus/session_manager_client.h"
#include "chromeos/dbus/shill_device_client.h"
#include "chromeos/dbus/shill_ipconfig_client.h"
#include "chromeos/dbus/shill_manager_client.h"
#include "chromeos/dbus/shill_profile_client.h"
#include "chromeos/dbus/shill_service_client.h"
#include "chromeos/dbus/shill_third_party_vpn_driver_client.h"
#include "chromeos/dbus/smb_provider_client.h"
#include "chromeos/dbus/sms_client.h"
#include "chromeos/dbus/system_clock_client.h"
#include "chromeos/dbus/update_engine_client.h"
#include "chromeos/dbus/upstart_client.h"
#include "dbus/bus.h"
#include "dbus/dbus_statistics.h"

namespace chromeos {

static DBusThreadManager* g_dbus_thread_manager = nullptr;
static bool g_using_dbus_thread_manager_for_testing = false;

DBusThreadManager::DBusThreadManager(ClientSet client_set,
                                     bool use_real_clients)
    : use_real_clients_(use_real_clients),
      clients_common_(new DBusClientsCommon(use_real_clients)) {
  if (client_set == DBusThreadManager::kAll)
    clients_browser_.reset(new DBusClientsBrowser(use_real_clients));
  // NOTE: When there are clients only used by ash, create them here.

  dbus::statistics::Initialize();

  if (use_real_clients) {
    // Create the D-Bus thread.
    base::Thread::Options thread_options;
    thread_options.message_loop_type = base::MessageLoop::TYPE_IO;
    dbus_thread_.reset(new base::Thread("D-Bus thread"));
    dbus_thread_->StartWithOptions(thread_options);

    // Create the connection to the system bus.
    dbus::Bus::Options system_bus_options;
    system_bus_options.bus_type = dbus::Bus::SYSTEM;
    system_bus_options.connection_type = dbus::Bus::PRIVATE;
    system_bus_options.dbus_task_runner = dbus_thread_->task_runner();
    system_bus_ = new dbus::Bus(system_bus_options);
  }
}

DBusThreadManager::~DBusThreadManager() {
  // Delete all D-Bus clients before shutting down the system bus.
  clients_browser_.reset();
  clients_common_.reset();

  // Shut down the bus. During the browser shutdown, it's ok to shut down
  // the bus synchronously.
  if (system_bus_.get())
    system_bus_->ShutdownOnDBusThreadAndBlock();

  // Stop the D-Bus thread.
  if (dbus_thread_)
    dbus_thread_->Stop();

  dbus::statistics::Shutdown();

  if (!g_dbus_thread_manager)
    return;  // Called form Shutdown() or local test instance.

  // There should never be both a global instance and a local instance.
  CHECK_EQ(this, g_dbus_thread_manager);
  if (g_using_dbus_thread_manager_for_testing) {
    g_dbus_thread_manager = nullptr;
    g_using_dbus_thread_manager_for_testing = false;
    VLOG(1) << "DBusThreadManager destroyed";
  } else {
    LOG(FATAL) << "~DBusThreadManager() called outside of Shutdown()";
  }
}

dbus::Bus* DBusThreadManager::GetSystemBus() {
  return system_bus_.get();
}

ArcAppfuseProviderClient* DBusThreadManager::GetArcAppfuseProviderClient() {
  return clients_browser_ ? clients_browser_->arc_appfuse_provider_client_.get()
                          : nullptr;
}

ArcMidisClient* DBusThreadManager::GetArcMidisClient() {
  return clients_browser_ ? clients_browser_->arc_midis_client_.get() : nullptr;
}

ArcObbMounterClient* DBusThreadManager::GetArcObbMounterClient() {
  return clients_browser_ ? clients_browser_->arc_obb_mounter_client_.get()
                          : nullptr;
}

ArcOemCryptoClient* DBusThreadManager::GetArcOemCryptoClient() {
  return clients_browser_ ? clients_browser_->arc_oemcrypto_client_.get()
                          : nullptr;
}

AuthPolicyClient* DBusThreadManager::GetAuthPolicyClient() {
  return clients_browser_ ? clients_browser_->auth_policy_client_.get()
                          : nullptr;
}

BiodClient* DBusThreadManager::GetBiodClient() {
  return clients_common_->biod_client_.get();
}

CecServiceClient* DBusThreadManager::GetCecServiceClient() {
  return clients_common_->cec_service_client_.get();
}

ConciergeClient* DBusThreadManager::GetConciergeClient() {
  return clients_browser_ ? clients_browser_->concierge_client_.get() : nullptr;
}

CrasAudioClient* DBusThreadManager::GetCrasAudioClient() {
  return clients_common_->cras_audio_client_.get();
}

CrosDisksClient* DBusThreadManager::GetCrosDisksClient() {
  return clients_browser_ ? clients_browser_->cros_disks_client_.get()
                          : nullptr;
}

CryptohomeClient* DBusThreadManager::GetCryptohomeClient() {
  return clients_common_->cryptohome_client_.get();
}

DebugDaemonClient* DBusThreadManager::GetDebugDaemonClient() {
  return clients_browser_ ? clients_browser_->debug_daemon_client_.get()
                          : nullptr;
}

EasyUnlockClient* DBusThreadManager::GetEasyUnlockClient() {
  return clients_browser_ ? clients_browser_->easy_unlock_client_.get()
                          : nullptr;
}

LorgnetteManagerClient* DBusThreadManager::GetLorgnetteManagerClient() {
  return clients_browser_ ? clients_browser_->lorgnette_manager_client_.get()
                          : nullptr;
}

ShillDeviceClient* DBusThreadManager::GetShillDeviceClient() {
  return clients_common_->shill_device_client_.get();
}

ShillIPConfigClient* DBusThreadManager::GetShillIPConfigClient() {
  return clients_common_->shill_ipconfig_client_.get();
}

ShillManagerClient* DBusThreadManager::GetShillManagerClient() {
  return clients_common_->shill_manager_client_.get();
}

ShillServiceClient* DBusThreadManager::GetShillServiceClient() {
  return clients_common_->shill_service_client_.get();
}

ShillProfileClient* DBusThreadManager::GetShillProfileClient() {
  return clients_common_->shill_profile_client_.get();
}

ShillThirdPartyVpnDriverClient*
DBusThreadManager::GetShillThirdPartyVpnDriverClient() {
  return clients_common_->shill_third_party_vpn_driver_client_.get();
}

GsmSMSClient* DBusThreadManager::GetGsmSMSClient() {
  return clients_common_->gsm_sms_client_.get();
}

HammerdClient* DBusThreadManager::GetHammerdClient() {
  return clients_common_->hammerd_client_.get();
}

ImageBurnerClient* DBusThreadManager::GetImageBurnerClient() {
  return clients_browser_ ? clients_browser_->image_burner_client_.get()
                          : nullptr;
}

ImageLoaderClient* DBusThreadManager::GetImageLoaderClient() {
  return clients_browser_ ? clients_browser_->image_loader_client_.get()
                          : nullptr;
}

MediaAnalyticsClient* DBusThreadManager::GetMediaAnalyticsClient() {
  return clients_browser_ ? clients_browser_->media_analytics_client_.get()
                          : nullptr;
}

ModemMessagingClient* DBusThreadManager::GetModemMessagingClient() {
  return clients_common_->modem_messaging_client_.get();
}

PermissionBrokerClient* DBusThreadManager::GetPermissionBrokerClient() {
  return clients_common_->permission_broker_client_.get();
}

PowerManagerClient* DBusThreadManager::GetPowerManagerClient() {
  return clients_common_->power_manager_client_.get();
}

SessionManagerClient* DBusThreadManager::GetSessionManagerClient() {
  return clients_common_->session_manager_client_.get();
}

SmbProviderClient* DBusThreadManager::GetSmbProviderClient() {
  return clients_browser_ ? clients_browser_->smb_provider_client_.get()
                          : nullptr;
}

SMSClient* DBusThreadManager::GetSMSClient() {
  return clients_common_->sms_client_.get();
}

SystemClockClient* DBusThreadManager::GetSystemClockClient() {
  return clients_common_->system_clock_client_.get();
}

UpdateEngineClient* DBusThreadManager::GetUpdateEngineClient() {
  return clients_common_->update_engine_client_.get();
}

UpstartClient* DBusThreadManager::GetUpstartClient() {
  return clients_common_ ? clients_common_->upstart_client_.get() : nullptr;
}

VirtualFileProviderClient* DBusThreadManager::GetVirtualFileProviderClient() {
  return clients_browser_
             ? clients_browser_->virtual_file_provider_client_.get()
             : nullptr;
}

void DBusThreadManager::InitializeClients() {
  // Some clients call DBusThreadManager::Get() during initialization.
  DCHECK(g_dbus_thread_manager);

  clients_common_->Initialize(GetSystemBus());
  if (clients_browser_)
    clients_browser_->Initialize(GetSystemBus());

  if (use_real_clients_)
    VLOG(1) << "DBusThreadManager initialized for Chrome OS";
  else
    VLOG(1) << "DBusThreadManager created for testing";
}

bool DBusThreadManager::IsUsingFakes() {
  return !use_real_clients_;
}

// static
void DBusThreadManager::Initialize(ClientSet client_set) {
  // If we initialize DBusThreadManager twice we may also be shutting it down
  // early; do not allow that.
  if (g_using_dbus_thread_manager_for_testing)
    return;

  CHECK(!g_dbus_thread_manager);
  bool use_real_clients = base::SysInfo::IsRunningOnChromeOS() &&
                          !base::CommandLine::ForCurrentProcess()->HasSwitch(
                              chromeos::switches::kDbusStub);
  g_dbus_thread_manager = new DBusThreadManager(client_set, use_real_clients);
  g_dbus_thread_manager->InitializeClients();
}

// static
void DBusThreadManager::Initialize() {
  Initialize(kAll);
}

// static
std::unique_ptr<DBusThreadManagerSetter>
DBusThreadManager::GetSetterForTesting() {
  if (!g_using_dbus_thread_manager_for_testing) {
    g_using_dbus_thread_manager_for_testing = true;
    CHECK(!g_dbus_thread_manager);
    // TODO(jamescook): Don't initialize clients as a side-effect of using a
    // test API. For now, assume the caller wants all clients.
    g_dbus_thread_manager =
        new DBusThreadManager(kAll, false /* use_real_clients */);
    g_dbus_thread_manager->InitializeClients();
  }

  return base::WrapUnique(new DBusThreadManagerSetter());
}

// static
bool DBusThreadManager::IsInitialized() {
  return !!g_dbus_thread_manager;
}

// static
void DBusThreadManager::Shutdown() {
  // Ensure that we only shutdown DBusThreadManager once.
  CHECK(g_dbus_thread_manager);
  DBusThreadManager* dbus_thread_manager = g_dbus_thread_manager;
  g_dbus_thread_manager = nullptr;
  g_using_dbus_thread_manager_for_testing = false;
  delete dbus_thread_manager;
  VLOG(1) << "DBusThreadManager Shutdown completed";
}

// static
DBusThreadManager* DBusThreadManager::Get() {
  CHECK(g_dbus_thread_manager)
      << "DBusThreadManager::Get() called before Initialize()";
  return g_dbus_thread_manager;
}

DBusThreadManagerSetter::DBusThreadManagerSetter() = default;

DBusThreadManagerSetter::~DBusThreadManagerSetter() = default;

void DBusThreadManagerSetter::SetAuthPolicyClient(
    std::unique_ptr<AuthPolicyClient> client) {
  DBusThreadManager::Get()->clients_browser_->auth_policy_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetBiodClient(
    std::unique_ptr<BiodClient> client) {
  DBusThreadManager::Get()->clients_common_->biod_client_ = std::move(client);
}

void DBusThreadManagerSetter::SetConciergeClient(
    std::unique_ptr<ConciergeClient> client) {
  DBusThreadManager::Get()->clients_browser_->concierge_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetCrasAudioClient(
    std::unique_ptr<CrasAudioClient> client) {
  DBusThreadManager::Get()->clients_common_->cras_audio_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetCrosDisksClient(
    std::unique_ptr<CrosDisksClient> client) {
  DBusThreadManager::Get()->clients_browser_->cros_disks_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetCryptohomeClient(
    std::unique_ptr<CryptohomeClient> client) {
  DBusThreadManager::Get()->clients_common_->cryptohome_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetDebugDaemonClient(
    std::unique_ptr<DebugDaemonClient> client) {
  DBusThreadManager::Get()->clients_browser_->debug_daemon_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetHammerdClient(
    std::unique_ptr<HammerdClient> client) {
  DBusThreadManager::Get()->clients_common_->hammerd_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetShillDeviceClient(
    std::unique_ptr<ShillDeviceClient> client) {
  DBusThreadManager::Get()->clients_common_->shill_device_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetShillIPConfigClient(
    std::unique_ptr<ShillIPConfigClient> client) {
  DBusThreadManager::Get()->clients_common_->shill_ipconfig_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetShillManagerClient(
    std::unique_ptr<ShillManagerClient> client) {
  DBusThreadManager::Get()->clients_common_->shill_manager_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetShillServiceClient(
    std::unique_ptr<ShillServiceClient> client) {
  DBusThreadManager::Get()->clients_common_->shill_service_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetShillProfileClient(
    std::unique_ptr<ShillProfileClient> client) {
  DBusThreadManager::Get()->clients_common_->shill_profile_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetShillThirdPartyVpnDriverClient(
    std::unique_ptr<ShillThirdPartyVpnDriverClient> client) {
  DBusThreadManager::Get()
      ->clients_common_->shill_third_party_vpn_driver_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetImageBurnerClient(
    std::unique_ptr<ImageBurnerClient> client) {
  DBusThreadManager::Get()->clients_browser_->image_burner_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetImageLoaderClient(
    std::unique_ptr<ImageLoaderClient> client) {
  DBusThreadManager::Get()->clients_browser_->image_loader_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetMediaAnalyticsClient(
    std::unique_ptr<MediaAnalyticsClient> client) {
  DBusThreadManager::Get()->clients_browser_->media_analytics_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetPermissionBrokerClient(
    std::unique_ptr<PermissionBrokerClient> client) {
  DBusThreadManager::Get()->clients_common_->permission_broker_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetPowerManagerClient(
    std::unique_ptr<PowerManagerClient> client) {
  DBusThreadManager::Get()->clients_common_->power_manager_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetSessionManagerClient(
    std::unique_ptr<SessionManagerClient> client) {
  DBusThreadManager::Get()->clients_common_->session_manager_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetSmbProviderClient(
    std::unique_ptr<SmbProviderClient> client) {
  DBusThreadManager::Get()->clients_browser_->smb_provider_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetSystemClockClient(
    std::unique_ptr<SystemClockClient> client) {
  DBusThreadManager::Get()->clients_common_->system_clock_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetUpdateEngineClient(
    std::unique_ptr<UpdateEngineClient> client) {
  DBusThreadManager::Get()->clients_common_->update_engine_client_ =
      std::move(client);
}

void DBusThreadManagerSetter::SetUpstartClient(
    std::unique_ptr<UpstartClient> client) {
  DBusThreadManager::Get()->clients_common_->upstart_client_ =
      std::move(client);
}

}  // namespace chromeos
