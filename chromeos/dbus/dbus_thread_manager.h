// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_DBUS_THREAD_MANAGER_H_
#define CHROMEOS_DBUS_DBUS_THREAD_MANAGER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chromeos/chromeos_export.h"

namespace base {
class Thread;
}  // namespace base

namespace dbus {
class Bus;
}  // namespace dbus

namespace chromeos {

// Style Note: Clients are sorted by names.
class ArcAppfuseProviderClient;
class ArcMidisClient;
class ArcObbMounterClient;
class ArcOemCryptoClient;
class AuthPolicyClient;
class BiodClient;
class CecServiceClient;
class ConciergeClient;
class CrasAudioClient;
class CrosDisksClient;
class CryptohomeClient;
class DBusClientsBrowser;
class DBusClientsCommon;
class DBusThreadManagerSetter;
class DebugDaemonClient;
class EasyUnlockClient;
class GsmSMSClient;
class HammerdClient;
class ImageBurnerClient;
class ImageLoaderClient;
class LorgnetteManagerClient;
class MediaAnalyticsClient;
class ModemMessagingClient;
class PermissionBrokerClient;
class PowerManagerClient;
class SMSClient;
class SessionManagerClient;
class ShillDeviceClient;
class ShillIPConfigClient;
class ShillManagerClient;
class ShillProfileClient;
class ShillServiceClient;
class ShillThirdPartyVpnDriverClient;
class SmbProviderClient;
class SystemClockClient;
class UpdateEngineClient;
class UpstartClient;
class VirtualFileProviderClient;

// DBusThreadManager manages the D-Bus thread, the thread dedicated to
// handling asynchronous D-Bus operations.
//
// This class also manages D-Bus connections and D-Bus clients, which
// depend on the D-Bus thread to ensure the right order of shutdowns for
// the D-Bus thread, the D-Bus connections, and the D-Bus clients.
//
// CALLBACKS IN D-BUS CLIENTS:
//
// D-Bus clients managed by DBusThreadManager are guaranteed to be deleted
// after the D-Bus thread so the clients don't need to worry if new
// incoming messages arrive from the D-Bus thread during shutdown of the
// clients. The UI message loop is not running during the shutdown hence
// the UI message loop won't post tasks to D-BUS clients during the
// shutdown. However, to be extra cautious, clients should use
// WeakPtrFactory when creating callbacks that run on UI thread. See
// session_manager_client.cc for examples.
//
class CHROMEOS_EXPORT DBusThreadManager {
 public:
  // Processes for which to create and initialize the D-Bus clients.
  // TODO(jamescook): Move creation of clients into //ash and //chrome/browser.
  // http://crbug.com/647367
  enum ClientSet {
    // Common clients needed by both ash and the browser.
    kShared,

    // Includes the client in |kShared| as well as the clients used only by
    // the browser (and not ash).
    kAll
  };
  // Sets the global instance. Must be called before any calls to Get().
  // We explicitly initialize and shut down the global object, rather than
  // making it a Singleton, to ensure clean startup and shutdown.
  // This will initialize real or fake DBusClients depending on command-line
  // arguments and whether this process runs in a ChromeOS environment.
  // Only D-Bus clients available in the processes in |process_mask| will be
  // created.
  static void Initialize(ClientSet client_set);

  // Equivalent to Initialize(kAll).
  static void Initialize();

  // Returns a DBusThreadManagerSetter instance that allows tests to
  // replace individual D-Bus clients with their own implementations.
  // Also initializes the main DBusThreadManager for testing if necessary.
  static std::unique_ptr<DBusThreadManagerSetter> GetSetterForTesting();

  // Returns true if DBusThreadManager has been initialized. Call this to
  // avoid initializing + shutting down DBusThreadManager more than once.
  static bool IsInitialized();

  // Destroys the global instance.
  static void Shutdown();

  // Gets the global instance. Initialize() must be called first.
  static DBusThreadManager* Get();

  // Returns true if clients are faked.
  bool IsUsingFakes();

  // Returns various D-Bus bus instances, owned by DBusThreadManager.
  dbus::Bus* GetSystemBus();

  // All returned objects are owned by DBusThreadManager.  Do not use these
  // pointers after DBusThreadManager has been shut down.
  // TODO(jamescook): Replace this with calls to FooClient::Get().
  // http://crbug.com/647367
  ArcAppfuseProviderClient* GetArcAppfuseProviderClient();
  ArcMidisClient* GetArcMidisClient();
  ArcObbMounterClient* GetArcObbMounterClient();
  ArcOemCryptoClient* GetArcOemCryptoClient();
  AuthPolicyClient* GetAuthPolicyClient();
  BiodClient* GetBiodClient();
  CecServiceClient* GetCecServiceClient();
  ConciergeClient* GetConciergeClient();
  CrasAudioClient* GetCrasAudioClient();
  CrosDisksClient* GetCrosDisksClient();
  CryptohomeClient* GetCryptohomeClient();
  DebugDaemonClient* GetDebugDaemonClient();
  EasyUnlockClient* GetEasyUnlockClient();
  GsmSMSClient* GetGsmSMSClient();
  HammerdClient* GetHammerdClient();
  ImageBurnerClient* GetImageBurnerClient();
  ImageLoaderClient* GetImageLoaderClient();
  MediaAnalyticsClient* GetMediaAnalyticsClient();
  LorgnetteManagerClient* GetLorgnetteManagerClient();
  ModemMessagingClient* GetModemMessagingClient();
  PermissionBrokerClient* GetPermissionBrokerClient();
  PowerManagerClient* GetPowerManagerClient();
  SessionManagerClient* GetSessionManagerClient();
  ShillDeviceClient* GetShillDeviceClient();
  ShillIPConfigClient* GetShillIPConfigClient();
  ShillManagerClient* GetShillManagerClient();
  ShillServiceClient* GetShillServiceClient();
  ShillProfileClient* GetShillProfileClient();
  ShillThirdPartyVpnDriverClient* GetShillThirdPartyVpnDriverClient();
  SmbProviderClient* GetSmbProviderClient();
  SMSClient* GetSMSClient();
  SystemClockClient* GetSystemClockClient();
  UpdateEngineClient* GetUpdateEngineClient();
  UpstartClient* GetUpstartClient();
  VirtualFileProviderClient* GetVirtualFileProviderClient();

 private:
  friend class DBusThreadManagerSetter;

  // Creates dbus clients based on |client_set|. Creates real clients if
  // |use_real_clients| is set, otherwise creates fakes.
  DBusThreadManager(ClientSet client_set, bool use_real_clients);
  ~DBusThreadManager();

  // Initializes all currently stored DBusClients with the system bus and
  // performs additional setup.
  void InitializeClients();

  std::unique_ptr<base::Thread> dbus_thread_;
  scoped_refptr<dbus::Bus> system_bus_;

  // Whether to use real or fake dbus clients.
  const bool use_real_clients_;

  // Clients used by multiple processes.
  std::unique_ptr<DBusClientsCommon> clients_common_;

  // Clients used only by the browser process. Null in other processes.
  std::unique_ptr<DBusClientsBrowser> clients_browser_;

  DISALLOW_COPY_AND_ASSIGN(DBusThreadManager);
};

// TODO(jamescook): Replace these with FooClient::InitializeForTesting().
class CHROMEOS_EXPORT DBusThreadManagerSetter {
 public:
  ~DBusThreadManagerSetter();

  void SetAuthPolicyClient(std::unique_ptr<AuthPolicyClient> client);
  void SetBiodClient(std::unique_ptr<BiodClient> client);
  void SetConciergeClient(std::unique_ptr<ConciergeClient> client);
  void SetCrasAudioClient(std::unique_ptr<CrasAudioClient> client);
  void SetCrosDisksClient(std::unique_ptr<CrosDisksClient> client);
  void SetCryptohomeClient(std::unique_ptr<CryptohomeClient> client);
  void SetDebugDaemonClient(std::unique_ptr<DebugDaemonClient> client);
  void SetHammerdClient(std::unique_ptr<HammerdClient> client);
  void SetShillDeviceClient(std::unique_ptr<ShillDeviceClient> client);
  void SetShillIPConfigClient(std::unique_ptr<ShillIPConfigClient> client);
  void SetShillManagerClient(std::unique_ptr<ShillManagerClient> client);
  void SetShillServiceClient(std::unique_ptr<ShillServiceClient> client);
  void SetShillProfileClient(std::unique_ptr<ShillProfileClient> client);
  void SetShillThirdPartyVpnDriverClient(
      std::unique_ptr<ShillThirdPartyVpnDriverClient> client);
  void SetImageBurnerClient(std::unique_ptr<ImageBurnerClient> client);
  void SetImageLoaderClient(std::unique_ptr<ImageLoaderClient> client);
  void SetMediaAnalyticsClient(std::unique_ptr<MediaAnalyticsClient> client);
  void SetPermissionBrokerClient(
      std::unique_ptr<PermissionBrokerClient> client);
  void SetPowerManagerClient(std::unique_ptr<PowerManagerClient> client);
  void SetSessionManagerClient(std::unique_ptr<SessionManagerClient> client);
  void SetSmbProviderClient(std::unique_ptr<SmbProviderClient> client);
  void SetSystemClockClient(std::unique_ptr<SystemClockClient> client);
  void SetUpdateEngineClient(std::unique_ptr<UpdateEngineClient> client);
  void SetUpstartClient(std::unique_ptr<UpstartClient> client);

 private:
  friend class DBusThreadManager;

  DBusThreadManagerSetter();

  DISALLOW_COPY_AND_ASSIGN(DBusThreadManagerSetter);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_DBUS_THREAD_MANAGER_H_
