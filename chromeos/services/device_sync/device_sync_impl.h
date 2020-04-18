// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_DEVICE_SYNC_DEVICE_SYNC_IMPL_H_
#define CHROMEOS_SERVICES_DEVICE_SYNC_DEVICE_SYNC_IMPL_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/services/device_sync/public/mojom/device_sync.mojom.h"
#include "components/cryptauth/cryptauth_enrollment_manager.h"
#include "components/cryptauth/cryptauth_gcm_manager.h"
#include "components/cryptauth/remote_device_provider.h"
#include "components/signin/core/browser/account_info.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/preferences/public/cpp/pref_service_factory.h"
#include "services/preferences/public/mojom/preferences.mojom.h"

class PrefService;

namespace base {
class Clock;
}  // namespace base

namespace cryptauth {
class CryptAuthClientFactory;
class CryptAuthDeviceManager;
class GcmDeviceInfoProvider;
class SoftwareFeatureManager;
}  // namespace cryptauth

namespace gcm {
class GCMDriver;
}  // namespace gcm

namespace identity {
class IdentityManager;
}  // namespace identity

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace service_manager {
class Connector;
}  // namespace service_manager

namespace chromeos {

namespace device_sync {

// Concrete DeviceSync implementation. When DeviceSyncImpl is constructed, it
// starts an initialization flow with the following steps:
// (1) Verify that the primary user is logged in with a valid account ID.
// (2) Connect to the Prefs Service associated with that account.
// (3) Instantiate classes which communicate with the CryptAuth back-end.
// (4) Check enrollment state; if not yet enrolled, enroll the device.
// (5) When enrollment is valid, listen for device sync updates.
class DeviceSyncImpl : public mojom::DeviceSync,
                       public cryptauth::CryptAuthEnrollmentManager::Observer,
                       public cryptauth::RemoteDeviceProvider::Observer {
 public:
  class Factory {
   public:
    static std::unique_ptr<DeviceSyncImpl> NewInstance(
        identity::IdentityManager* identity_manager,
        gcm::GCMDriver* gcm_driver,
        service_manager::Connector* connector,
        cryptauth::GcmDeviceInfoProvider* gcm_device_info_provider,
        scoped_refptr<net::URLRequestContextGetter> url_request_context);
    static void SetInstanceForTesting(Factory* test_factory);

    virtual ~Factory();
    virtual std::unique_ptr<DeviceSyncImpl> BuildInstance(
        identity::IdentityManager* identity_manager,
        gcm::GCMDriver* gcm_driver,
        service_manager::Connector* connector,
        cryptauth::GcmDeviceInfoProvider* gcm_device_info_provider,
        scoped_refptr<net::URLRequestContextGetter> url_request_context);

   private:
    static Factory* test_factory_instance_;
  };

  ~DeviceSyncImpl() override;

  // Binds a request to this implementation. Should be called each time that the
  // service receives a request.
  void BindRequest(mojom::DeviceSyncRequest request);

 protected:
  // mojom::DeviceSync:
  void AddObserver(mojom::DeviceSyncObserverPtr observer,
                   AddObserverCallback callback) override;
  void ForceEnrollmentNow(ForceEnrollmentNowCallback callback) override;
  void ForceSyncNow(ForceSyncNowCallback callback) override;
  void GetLocalDeviceMetadata(GetLocalDeviceMetadataCallback callback) override;
  void GetSyncedDevices(GetSyncedDevicesCallback callback) override;
  void SetSoftwareFeatureState(
      const std::string& device_public_key,
      cryptauth::SoftwareFeature software_feature,
      bool enabled,
      bool is_exclusive,
      SetSoftwareFeatureStateCallback callback) override;
  void FindEligibleDevices(cryptauth::SoftwareFeature software_feature,
                           FindEligibleDevicesCallback callback) override;
  void GetDebugInfo(GetDebugInfoCallback callback) override;

  // cryptauth::CryptAuthEnrollmentManager::Observer:
  void OnEnrollmentFinished(bool success) override;

  // cryptauth::RemoteDeviceProvider::Observer:
  void OnSyncDeviceListChanged() override;

 private:
  friend class DeviceSyncServiceTest;

  enum class Status {
    FETCHING_ACCOUNT_INFO,
    CONNECTING_TO_USER_PREFS,
    WAITING_FOR_ENROLLMENT,
    READY
  };

  // Wrapper around preferences code. This class is necessary so that tests can
  // override this functionality to use a fake PrefService rather than a real
  // connection to the Preferences service.
  class PrefConnectionDelegate {
   public:
    virtual ~PrefConnectionDelegate();

    virtual scoped_refptr<PrefRegistrySimple> CreatePrefRegistry();
    virtual void ConnectToPrefService(
        service_manager::Connector* connector,
        scoped_refptr<PrefRegistrySimple> pref_registry,
        prefs::ConnectCallback callback);
  };

  DeviceSyncImpl(
      identity::IdentityManager* identity_manager,
      gcm::GCMDriver* gcm_driver,
      service_manager::Connector* connector,
      cryptauth::GcmDeviceInfoProvider* gcm_device_info_provider,
      scoped_refptr<net::URLRequestContextGetter> url_request_context,
      base::Clock* clock,
      std::unique_ptr<PrefConnectionDelegate> pref_connection_delegate);

  void ProcessPrimaryAccountInfo(const AccountInfo& primary_account_info);
  void ConnectToPrefStore();
  void OnConnectedToPrefService(std::unique_ptr<PrefService> pref_service);
  void InitializeCryptAuthManagementObjects();
  void CompleteInitializationAfterSuccessfulEnrollment();

  base::Optional<cryptauth::RemoteDevice> GetSyncedDeviceWithPublicKey(
      const std::string& public_key) const;

  void OnSetSoftwareFeatureStateSuccess(
      const base::RepeatingCallback<void(const base::Optional<std::string>&)>&
          callback);
  void OnSetSoftwareFeatureStateError(
      const base::RepeatingCallback<void(const base::Optional<std::string>&)>&
          callback,
      const std::string& error);
  void OnFindEligibleDevicesSuccess(
      const base::RepeatingCallback<
          void(const base::Optional<std::string>&,
               mojom::FindEligibleDevicesResponsePtr)>& callback,
      const std::vector<cryptauth::ExternalDeviceInfo>& eligible_devices,
      const std::vector<cryptauth::IneligibleDevice>& ineligible_devices);
  void OnFindEligibleDevicesError(
      const base::RepeatingCallback<
          void(const base::Optional<std::string>&,
               mojom::FindEligibleDevicesResponsePtr)>& callback,
      const std::string& error);

  void SetPrefConnectionDelegateForTesting(
      std::unique_ptr<PrefConnectionDelegate> pref_connection_delegate);

  identity::IdentityManager* identity_manager_;
  gcm::GCMDriver* gcm_driver_;
  service_manager::Connector* connector_;
  cryptauth::GcmDeviceInfoProvider* gcm_device_info_provider_;
  scoped_refptr<net::URLRequestContextGetter> url_request_context_;
  base::Clock* clock_;
  std::unique_ptr<PrefConnectionDelegate> pref_connection_delegate_;

  Status status_;
  AccountInfo primary_account_info_;
  std::unique_ptr<PrefService> pref_service_;

  std::unique_ptr<cryptauth::CryptAuthGCMManager> cryptauth_gcm_manager_;
  std::unique_ptr<cryptauth::CryptAuthClientFactory> cryptauth_client_factory_;
  std::unique_ptr<cryptauth::CryptAuthEnrollmentManager>
      cryptauth_enrollment_manager_;
  std::unique_ptr<cryptauth::CryptAuthDeviceManager> cryptauth_device_manager_;
  std::unique_ptr<cryptauth::RemoteDeviceProvider> remote_device_provider_;
  std::unique_ptr<cryptauth::SoftwareFeatureManager> software_feature_manager_;

  mojo::InterfacePtrSet<mojom::DeviceSyncObserver> observers_;
  mojo::BindingSet<mojom::DeviceSync> bindings_;

  base::WeakPtrFactory<DeviceSyncImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DeviceSyncImpl);
};

}  // namespace device_sync

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_DEVICE_SYNC_DEVICE_SYNC_IMPL_H_
