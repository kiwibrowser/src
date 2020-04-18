// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/dbus_clients_common.h"

#include "base/command_line.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/biod/biod_client.h"
#include "chromeos/dbus/cec_service_client.h"
#include "chromeos/dbus/cras_audio_client.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/dbus_client_implementation_type.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_cras_audio_client.h"
#include "chromeos/dbus/fake_cryptohome_client.h"
#include "chromeos/dbus/fake_gsm_sms_client.h"
#include "chromeos/dbus/fake_hammerd_client.h"
#include "chromeos/dbus/fake_modem_messaging_client.h"
#include "chromeos/dbus/fake_permission_broker_client.h"
#include "chromeos/dbus/fake_shill_device_client.h"
#include "chromeos/dbus/fake_shill_ipconfig_client.h"
#include "chromeos/dbus/fake_shill_manager_client.h"
#include "chromeos/dbus/fake_shill_profile_client.h"
#include "chromeos/dbus/fake_shill_service_client.h"
#include "chromeos/dbus/fake_shill_third_party_vpn_driver_client.h"
#include "chromeos/dbus/fake_sms_client.h"
#include "chromeos/dbus/fake_system_clock_client.h"
#include "chromeos/dbus/fake_upstart_client.h"
#include "chromeos/dbus/gsm_sms_client.h"
#include "chromeos/dbus/hammerd_client.h"
#include "chromeos/dbus/modem_messaging_client.h"
#include "chromeos/dbus/permission_broker_client.h"
#include "chromeos/dbus/power_manager_client.h"
#include "chromeos/dbus/power_policy_controller.h"
#include "chromeos/dbus/session_manager_client.h"
#include "chromeos/dbus/shill_device_client.h"
#include "chromeos/dbus/shill_ipconfig_client.h"
#include "chromeos/dbus/shill_manager_client.h"
#include "chromeos/dbus/shill_profile_client.h"
#include "chromeos/dbus/shill_service_client.h"
#include "chromeos/dbus/shill_third_party_vpn_driver_client.h"
#include "chromeos/dbus/sms_client.h"
#include "chromeos/dbus/system_clock_client.h"
#include "chromeos/dbus/update_engine_client.h"
#include "chromeos/dbus/upstart_client.h"

namespace chromeos {

DBusClientsCommon::DBusClientsCommon(bool use_real_clients) {
  const DBusClientImplementationType client_impl_type =
      use_real_clients ? REAL_DBUS_CLIENT_IMPLEMENTATION
                       : FAKE_DBUS_CLIENT_IMPLEMENTATION;

  biod_client_.reset(BiodClient::Create(client_impl_type));

  cec_service_client_ = CecServiceClient::Create(client_impl_type);

  if (use_real_clients)
    cras_audio_client_.reset(CrasAudioClient::Create());
  else
    cras_audio_client_.reset(new FakeCrasAudioClient);

  if (use_real_clients)
    cryptohome_client_.reset(CryptohomeClient::Create());
  else
    cryptohome_client_.reset(new FakeCryptohomeClient);

  if (use_real_clients) {
    shill_manager_client_.reset(ShillManagerClient::Create());
    shill_device_client_.reset(ShillDeviceClient::Create());
    shill_ipconfig_client_.reset(ShillIPConfigClient::Create());
    shill_service_client_.reset(ShillServiceClient::Create());
    shill_profile_client_.reset(ShillProfileClient::Create());
    shill_third_party_vpn_driver_client_.reset(
        ShillThirdPartyVpnDriverClient::Create());
  } else {
    shill_manager_client_.reset(new FakeShillManagerClient);
    shill_device_client_.reset(new FakeShillDeviceClient);
    shill_ipconfig_client_.reset(new FakeShillIPConfigClient);
    shill_service_client_.reset(new FakeShillServiceClient);
    shill_profile_client_.reset(new FakeShillProfileClient);
    shill_third_party_vpn_driver_client_.reset(
        new FakeShillThirdPartyVpnDriverClient);
  }

  if (use_real_clients) {
    gsm_sms_client_.reset(GsmSMSClient::Create());
  } else {
    FakeGsmSMSClient* gsm_sms_client = new FakeGsmSMSClient();
    gsm_sms_client->set_sms_test_message_switch_present(
        base::CommandLine::ForCurrentProcess()->HasSwitch(
            chromeos::switches::kSmsTestMessages));
    gsm_sms_client_.reset(gsm_sms_client);
  }

  if (use_real_clients) {
    hammerd_client_ = HammerdClient::Create();
  } else {
    hammerd_client_ = std::make_unique<FakeHammerdClient>();
  }

  if (use_real_clients)
    modem_messaging_client_.reset(ModemMessagingClient::Create());
  else
    modem_messaging_client_.reset(new FakeModemMessagingClient);

  if (use_real_clients)
    permission_broker_client_.reset(PermissionBrokerClient::Create());
  else
    permission_broker_client_.reset(new FakePermissionBrokerClient);

  power_manager_client_.reset(PowerManagerClient::Create(client_impl_type));

  session_manager_client_.reset(SessionManagerClient::Create(client_impl_type));

  if (use_real_clients)
    sms_client_.reset(SMSClient::Create());
  else
    sms_client_.reset(new FakeSMSClient);

  if (use_real_clients)
    system_clock_client_.reset(SystemClockClient::Create());
  else
    system_clock_client_.reset(new FakeSystemClockClient);

  update_engine_client_.reset(UpdateEngineClient::Create(client_impl_type));

  if (use_real_clients)
    upstart_client_.reset(UpstartClient::Create());
  else
    upstart_client_.reset(new FakeUpstartClient);
}

DBusClientsCommon::~DBusClientsCommon() = default;

void DBusClientsCommon::Initialize(dbus::Bus* system_bus) {
  DCHECK(DBusThreadManager::IsInitialized());

  biod_client_->Init(system_bus);
  cec_service_client_->Init(system_bus);
  cras_audio_client_->Init(system_bus);
  cryptohome_client_->Init(system_bus);
  gsm_sms_client_->Init(system_bus);
  hammerd_client_->Init(system_bus);
  modem_messaging_client_->Init(system_bus);
  permission_broker_client_->Init(system_bus);
  power_manager_client_->Init(system_bus);
  session_manager_client_->Init(system_bus);
  shill_device_client_->Init(system_bus);
  shill_ipconfig_client_->Init(system_bus);
  shill_manager_client_->Init(system_bus);
  shill_service_client_->Init(system_bus);
  shill_profile_client_->Init(system_bus);
  shill_third_party_vpn_driver_client_->Init(system_bus);
  sms_client_->Init(system_bus);
  system_clock_client_->Init(system_bus);
  update_engine_client_->Init(system_bus);
  upstart_client_->Init(system_bus);

  ShillManagerClient::TestInterface* manager =
      shill_manager_client_->GetTestInterface();
  if (manager)
    manager->SetupDefaultEnvironment();
}

}  // namespace chromeos
