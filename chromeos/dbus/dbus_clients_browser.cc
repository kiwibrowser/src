// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/dbus_clients_browser.h"

#include "base/logging.h"
#include "chromeos/dbus/arc_appfuse_provider_client.h"
#include "chromeos/dbus/arc_midis_client.h"
#include "chromeos/dbus/arc_obb_mounter_client.h"
#include "chromeos/dbus/arc_oemcrypto_client.h"
#include "chromeos/dbus/auth_policy_client.h"
#include "chromeos/dbus/concierge_client.h"
#include "chromeos/dbus/cros_disks_client.h"
#include "chromeos/dbus/dbus_client_implementation_type.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/debug_daemon_client.h"
#include "chromeos/dbus/easy_unlock_client.h"
#include "chromeos/dbus/fake_arc_appfuse_provider_client.h"
#include "chromeos/dbus/fake_arc_midis_client.h"
#include "chromeos/dbus/fake_arc_obb_mounter_client.h"
#include "chromeos/dbus/fake_arc_oemcrypto_client.h"
#include "chromeos/dbus/fake_auth_policy_client.h"
#include "chromeos/dbus/fake_concierge_client.h"
#include "chromeos/dbus/fake_debug_daemon_client.h"
#include "chromeos/dbus/fake_easy_unlock_client.h"
#include "chromeos/dbus/fake_image_burner_client.h"
#include "chromeos/dbus/fake_image_loader_client.h"
#include "chromeos/dbus/fake_lorgnette_manager_client.h"
#include "chromeos/dbus/fake_media_analytics_client.h"
#include "chromeos/dbus/fake_smb_provider_client.h"
#include "chromeos/dbus/fake_virtual_file_provider_client.h"
#include "chromeos/dbus/image_burner_client.h"
#include "chromeos/dbus/image_loader_client.h"
#include "chromeos/dbus/lorgnette_manager_client.h"
#include "chromeos/dbus/media_analytics_client.h"
#include "chromeos/dbus/smb_provider_client.h"
#include "chromeos/dbus/virtual_file_provider_client.h"

namespace chromeos {

DBusClientsBrowser::DBusClientsBrowser(bool use_real_clients) {
  if (use_real_clients) {
    arc_appfuse_provider_client_ = ArcAppfuseProviderClient::Create();
  } else {
    arc_appfuse_provider_client_ =
        std::make_unique<FakeArcAppfuseProviderClient>();
  }

  if (use_real_clients)
    arc_midis_client_ = ArcMidisClient::Create();
  else
    arc_midis_client_.reset(new FakeArcMidisClient);

  if (use_real_clients)
    arc_obb_mounter_client_.reset(ArcObbMounterClient::Create());
  else
    arc_obb_mounter_client_.reset(new FakeArcObbMounterClient);

  if (use_real_clients)
    arc_oemcrypto_client_.reset(ArcOemCryptoClient::Create());
  else
    arc_oemcrypto_client_.reset(new FakeArcOemCryptoClient);

  if (use_real_clients)
    auth_policy_client_.reset(AuthPolicyClient::Create());
  else
    auth_policy_client_.reset(new FakeAuthPolicyClient);

  cros_disks_client_.reset(CrosDisksClient::Create(
      use_real_clients ? REAL_DBUS_CLIENT_IMPLEMENTATION
                       : FAKE_DBUS_CLIENT_IMPLEMENTATION));

  if (use_real_clients)
    concierge_client_.reset(ConciergeClient::Create());
  else
    concierge_client_.reset(new FakeConciergeClient);

  if (use_real_clients)
    debug_daemon_client_.reset(DebugDaemonClient::Create());
  else
    debug_daemon_client_.reset(new FakeDebugDaemonClient);

  if (use_real_clients)
    easy_unlock_client_.reset(EasyUnlockClient::Create());
  else
    easy_unlock_client_.reset(new FakeEasyUnlockClient);

  if (use_real_clients)
    image_burner_client_.reset(ImageBurnerClient::Create());
  else
    image_burner_client_.reset(new FakeImageBurnerClient);

  if (use_real_clients)
    image_loader_client_.reset(ImageLoaderClient::Create());
  else
    image_loader_client_.reset(new FakeImageLoaderClient);

  if (use_real_clients)
    lorgnette_manager_client_.reset(LorgnetteManagerClient::Create());
  else
    lorgnette_manager_client_.reset(new FakeLorgnetteManagerClient);

  if (use_real_clients)
    media_analytics_client_.reset(MediaAnalyticsClient::Create());
  else
    media_analytics_client_.reset(new FakeMediaAnalyticsClient);

  if (use_real_clients)
    smb_provider_client_.reset(SmbProviderClient::Create());
  else
    smb_provider_client_ = std::make_unique<FakeSmbProviderClient>();

  if (use_real_clients)
    virtual_file_provider_client_.reset(VirtualFileProviderClient::Create());
  else
    virtual_file_provider_client_.reset(new FakeVirtualFileProviderClient);
}

DBusClientsBrowser::~DBusClientsBrowser() = default;

void DBusClientsBrowser::Initialize(dbus::Bus* system_bus) {
  DCHECK(DBusThreadManager::IsInitialized());

  arc_midis_client_->Init(system_bus);
  arc_obb_mounter_client_->Init(system_bus);
  arc_oemcrypto_client_->Init(system_bus);
  auth_policy_client_->Init(system_bus);
  concierge_client_->Init(system_bus);
  cros_disks_client_->Init(system_bus);
  debug_daemon_client_->Init(system_bus);
  easy_unlock_client_->Init(system_bus);
  image_burner_client_->Init(system_bus);
  image_loader_client_->Init(system_bus);
  lorgnette_manager_client_->Init(system_bus);
  media_analytics_client_->Init(system_bus);
  smb_provider_client_->Init(system_bus);
  virtual_file_provider_client_->Init(system_bus);
}

}  // namespace chromeos
