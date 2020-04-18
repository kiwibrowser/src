// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_DBUS_CLIENTS_BROWSER_H_
#define CHROMEOS_DBUS_DBUS_CLIENTS_BROWSER_H_

#include <memory>

#include "base/macros.h"
#include "chromeos/chromeos_export.h"

namespace dbus {
class Bus;
}

namespace chromeos {

class ArcAppfuseProviderClient;
class ArcMidisClient;
class ArcObbMounterClient;
class ArcOemCryptoClient;
class AuthPolicyClient;
class ConciergeClient;
class CrosDisksClient;
class DebugDaemonClient;
class EasyUnlockClient;
class ImageBurnerClient;
class ImageLoaderClient;
class LorgnetteManagerClient;
class MediaAnalyticsClient;
class SmbProviderClient;
class VirtualFileProviderClient;

// D-Bus clients used only in the browser process.
// TODO(jamescook): Move this under //chrome/browser. http://crbug.com/647367
class CHROMEOS_EXPORT DBusClientsBrowser {
 public:
  // Creates real implementations if |use_real_clients| is true and fakes
  // otherwise. Fakes are used when running on Linux desktop and in tests.
  explicit DBusClientsBrowser(bool use_real_clients);
  ~DBusClientsBrowser();

  void Initialize(dbus::Bus* system_bus);

 private:
  friend class DBusThreadManager;
  friend class DBusThreadManagerSetter;

  std::unique_ptr<ArcAppfuseProviderClient> arc_appfuse_provider_client_;
  std::unique_ptr<ArcMidisClient> arc_midis_client_;
  std::unique_ptr<ArcObbMounterClient> arc_obb_mounter_client_;
  std::unique_ptr<ArcOemCryptoClient> arc_oemcrypto_client_;
  std::unique_ptr<AuthPolicyClient> auth_policy_client_;
  std::unique_ptr<ConciergeClient> concierge_client_;
  std::unique_ptr<CrosDisksClient> cros_disks_client_;
  std::unique_ptr<DebugDaemonClient> debug_daemon_client_;
  std::unique_ptr<EasyUnlockClient> easy_unlock_client_;
  std::unique_ptr<ImageBurnerClient> image_burner_client_;
  std::unique_ptr<ImageLoaderClient> image_loader_client_;
  std::unique_ptr<LorgnetteManagerClient> lorgnette_manager_client_;
  std::unique_ptr<MediaAnalyticsClient> media_analytics_client_;
  std::unique_ptr<SmbProviderClient> smb_provider_client_;
  std::unique_ptr<VirtualFileProviderClient> virtual_file_provider_client_;

  DISALLOW_COPY_AND_ASSIGN(DBusClientsBrowser);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_DBUS_CLIENTS_BROWSER_H_
