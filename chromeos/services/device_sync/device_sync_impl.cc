// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/device_sync/device_sync_impl.h"

#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "base/optional.h"
#include "base/time/default_clock.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/services/device_sync/cryptauth_client_factory_impl.h"
#include "chromeos/services/device_sync/cryptauth_enroller_factory_impl.h"
#include "components/cryptauth/cryptauth_device_manager_impl.h"
#include "components/cryptauth/cryptauth_enrollment_manager_impl.h"
#include "components/cryptauth/cryptauth_gcm_manager_impl.h"
#include "components/cryptauth/device_classifier_util.h"
#include "components/cryptauth/gcm_device_info_provider.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device_provider_impl.h"
#include "components/cryptauth/secure_message_delegate_impl.h"
#include "components/cryptauth/software_feature_manager_impl.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/identity/public/cpp/identity_manager.h"
#include "services/service_manager/public/cpp/connector.h"

namespace chromeos {

namespace device_sync {

namespace {

void RegisterDeviceSyncPrefs(PrefRegistrySimple* registry) {
  cryptauth::CryptAuthGCMManager::RegisterPrefs(registry);
  cryptauth::CryptAuthDeviceManager::RegisterPrefs(registry);
  cryptauth::CryptAuthEnrollmentManager::RegisterPrefs(registry);
}

}  // namespace

// static
DeviceSyncImpl::Factory* DeviceSyncImpl::Factory::test_factory_instance_ =
    nullptr;

// static
std::unique_ptr<DeviceSyncImpl> DeviceSyncImpl::Factory::NewInstance(
    identity::IdentityManager* identity_manager,
    gcm::GCMDriver* gcm_driver,
    service_manager::Connector* connector,
    cryptauth::GcmDeviceInfoProvider* gcm_device_info_provider,
    scoped_refptr<net::URLRequestContextGetter> url_request_context) {
  if (test_factory_instance_) {
    return test_factory_instance_->BuildInstance(
        identity_manager, gcm_driver, connector, gcm_device_info_provider,
        std::move(url_request_context));
  }

  static base::NoDestructor<DeviceSyncImpl::Factory> default_factory;
  return default_factory->BuildInstance(identity_manager, gcm_driver, connector,
                                        gcm_device_info_provider,
                                        std::move(url_request_context));
}

// static
void DeviceSyncImpl::Factory::SetInstanceForTesting(Factory* test_factory) {
  test_factory_instance_ = test_factory;
}

DeviceSyncImpl::Factory::~Factory() = default;

std::unique_ptr<DeviceSyncImpl> DeviceSyncImpl::Factory::BuildInstance(
    identity::IdentityManager* identity_manager,
    gcm::GCMDriver* gcm_driver,
    service_manager::Connector* connector,
    cryptauth::GcmDeviceInfoProvider* gcm_device_info_provider,
    scoped_refptr<net::URLRequestContextGetter> url_request_context) {
  return base::WrapUnique(new DeviceSyncImpl(
      identity_manager, gcm_driver, connector, gcm_device_info_provider,
      std::move(url_request_context), base::DefaultClock::GetInstance(),
      std::make_unique<PrefConnectionDelegate>()));
}

DeviceSyncImpl::PrefConnectionDelegate::~PrefConnectionDelegate() = default;

scoped_refptr<PrefRegistrySimple>
DeviceSyncImpl::PrefConnectionDelegate::CreatePrefRegistry() {
  return base::MakeRefCounted<PrefRegistrySimple>();
}

void DeviceSyncImpl::PrefConnectionDelegate::ConnectToPrefService(
    service_manager::Connector* connector,
    scoped_refptr<PrefRegistrySimple> pref_registry,
    prefs::ConnectCallback callback) {
  prefs::mojom::PrefStoreConnectorPtr pref_store_connector;
  connector->BindInterface(prefs::mojom::kServiceName, &pref_store_connector);
  prefs::ConnectToPrefService(std::move(pref_store_connector),
                              std::move(pref_registry), std::move(callback));
}

DeviceSyncImpl::DeviceSyncImpl(
    identity::IdentityManager* identity_manager,
    gcm::GCMDriver* gcm_driver,
    service_manager::Connector* connector,
    cryptauth::GcmDeviceInfoProvider* gcm_device_info_provider,
    scoped_refptr<net::URLRequestContextGetter> url_request_context,
    base::Clock* clock,
    std::unique_ptr<PrefConnectionDelegate> pref_connection_delegate)
    : identity_manager_(identity_manager),
      gcm_driver_(gcm_driver),
      connector_(connector),
      gcm_device_info_provider_(gcm_device_info_provider),
      url_request_context_(url_request_context),
      clock_(clock),
      pref_connection_delegate_(std::move(pref_connection_delegate)),
      status_(Status::FETCHING_ACCOUNT_INFO),
      weak_ptr_factory_(this) {
  PA_LOG(INFO) << "DeviceSyncImpl: Initializing.";
  ProcessPrimaryAccountInfo(identity_manager_->GetPrimaryAccountInfo());
}

DeviceSyncImpl::~DeviceSyncImpl() {
  if (cryptauth_enrollment_manager_)
    cryptauth_enrollment_manager_->RemoveObserver(this);

  if (remote_device_provider_)
    remote_device_provider_->RemoveObserver(this);
}

void DeviceSyncImpl::BindRequest(mojom::DeviceSyncRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void DeviceSyncImpl::AddObserver(mojom::DeviceSyncObserverPtr observer,
                                 AddObserverCallback callback) {
  observers_.AddPtr(std::move(observer));
  std::move(callback).Run();
}

void DeviceSyncImpl::ForceEnrollmentNow(ForceEnrollmentNowCallback callback) {
  if (status_ != Status::READY) {
    PA_LOG(WARNING) << "DeviceSyncImpl::ForceEnrollmentNow() invoked before "
                    << "initialization was complete. Cannot force enrollment.";
    std::move(callback).Run(false /* success */);
    return;
  }

  cryptauth_enrollment_manager_->ForceEnrollmentNow(
      cryptauth::INVOCATION_REASON_MANUAL);
  std::move(callback).Run(true /* success */);
}

void DeviceSyncImpl::ForceSyncNow(ForceSyncNowCallback callback) {
  if (status_ != Status::READY) {
    PA_LOG(WARNING) << "DeviceSyncImpl::ForceSyncNow() invoked before "
                    << "initialization was complete. Cannot force sync.";
    std::move(callback).Run(false /* success */);
    return;
  }

  cryptauth_device_manager_->ForceSyncNow(cryptauth::INVOCATION_REASON_MANUAL);
  std::move(callback).Run(true /* success */);
}

void DeviceSyncImpl::GetLocalDeviceMetadata(
    GetLocalDeviceMetadataCallback callback) {
  if (status_ != Status::READY) {
    PA_LOG(WARNING) << "DeviceSyncImpl::GetLocalDeviceMetadata() invoked "
                    << "before initialization was complete. Cannot return "
                    << "local device metadata.";
    std::move(callback).Run(base::nullopt);
    return;
  }

  std::string public_key = cryptauth_enrollment_manager_->GetUserPublicKey();
  DCHECK(!public_key.empty());
  std::move(callback).Run(GetSyncedDeviceWithPublicKey(public_key));
}

void DeviceSyncImpl::GetSyncedDevices(GetSyncedDevicesCallback callback) {
  if (status_ != Status::READY) {
    PA_LOG(WARNING) << "DeviceSyncImpl::GetSyncedDevices() invoked before "
                    << "initialization was complete. Cannot return devices.";
    std::move(callback).Run(base::nullopt);
    return;
  }

  std::move(callback).Run(remote_device_provider_->GetSyncedDevices());
}

void DeviceSyncImpl::SetSoftwareFeatureState(
    const std::string& device_public_key,
    cryptauth::SoftwareFeature software_feature,
    bool enabled,
    bool is_exclusive,
    SetSoftwareFeatureStateCallback callback) {
  if (status_ != Status::READY) {
    PA_LOG(WARNING) << "DeviceSyncImpl::SetSoftwareFeatureState() invoked "
                    << "before initialization was complete. Cannot set state.";
    std::move(callback).Run(mojom::kErrorNotInitialized);
    return;
  }

  auto callback_holder = base::AdaptCallbackForRepeating(std::move(callback));
  software_feature_manager_->SetSoftwareFeatureState(
      device_public_key, software_feature, enabled,
      base::Bind(&DeviceSyncImpl::OnSetSoftwareFeatureStateSuccess,
                 weak_ptr_factory_.GetWeakPtr(), callback_holder),
      base::Bind(&DeviceSyncImpl::OnSetSoftwareFeatureStateError,
                 weak_ptr_factory_.GetWeakPtr(), callback_holder),
      is_exclusive);
}

void DeviceSyncImpl::FindEligibleDevices(
    cryptauth::SoftwareFeature software_feature,
    FindEligibleDevicesCallback callback) {
  if (status_ != Status::READY) {
    PA_LOG(WARNING) << "DeviceSyncImpl::FindEligibleDevices() invoked before "
                    << "initialization was complete. Cannot find devices.";
    std::move(callback).Run(mojom::kErrorNotInitialized,
                            nullptr /* response */);
    return;
  }

  auto callback_holder = base::AdaptCallbackForRepeating(std::move(callback));
  software_feature_manager_->FindEligibleDevices(
      software_feature,
      base::Bind(&DeviceSyncImpl::OnFindEligibleDevicesSuccess,
                 weak_ptr_factory_.GetWeakPtr(), callback_holder),
      base::Bind(&DeviceSyncImpl::OnFindEligibleDevicesError,
                 weak_ptr_factory_.GetWeakPtr(), callback_holder));
}

void DeviceSyncImpl::GetDebugInfo(GetDebugInfoCallback callback) {
  if (status_ != Status::READY) {
    PA_LOG(WARNING) << "DeviceSyncImpl::GetDebugInfo() invoked before "
                    << "initialization was complete. Cannot provide info.";
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(mojom::DebugInfo::New(
      cryptauth_enrollment_manager_->GetLastEnrollmentTime(),
      cryptauth_enrollment_manager_->GetTimeToNextAttempt(),
      cryptauth_enrollment_manager_->IsRecoveringFromFailure(),
      cryptauth_enrollment_manager_->IsEnrollmentInProgress(),
      cryptauth_device_manager_->GetLastSyncTime(),
      cryptauth_device_manager_->GetTimeToNextAttempt(),
      cryptauth_device_manager_->IsRecoveringFromFailure(),
      cryptauth_device_manager_->IsSyncInProgress()));
}

void DeviceSyncImpl::OnEnrollmentFinished(bool success) {
  PA_LOG(INFO) << "DeviceSyncImpl: Enrollment finished; success = " << success;

  if (!success)
    return;

  if (status_ == Status::WAITING_FOR_ENROLLMENT)
    CompleteInitializationAfterSuccessfulEnrollment();

  observers_.ForAllPtrs(
      [](auto* observer) { observer->OnEnrollmentFinished(); });
}

void DeviceSyncImpl::OnSyncDeviceListChanged() {
  PA_LOG(INFO) << "DeviceSyncImpl: Synced devices changed; notifying "
               << "observers.";
  observers_.ForAllPtrs([](auto* observer) { observer->OnNewDevicesSynced(); });
}

void DeviceSyncImpl::ProcessPrimaryAccountInfo(
    const AccountInfo& primary_account_info) {
  // Note: We cannot use |primary_account_info.IsValid()| here because
  //       IdentityTestEnvironment is buggy: https://crbug.com/830122. For now,
  //       we simply check that the account ID is set.
  // TODO(khorimoto): Use IsValid() once the aforementioned bug is fixed.
  if (primary_account_info.account_id.empty()) {
    PA_LOG(ERROR) << "Primary account information is invalid; cannot proceed.";

    // This situation should never occur in practice. The log above is added to
    // ensure that this is flagged in release builds where NOTREACHED() does not
    // crash the process.
    NOTREACHED();
    return;
  }

  primary_account_info_ = primary_account_info;
  ConnectToPrefStore();
}

void DeviceSyncImpl::ConnectToPrefStore() {
  DCHECK(status_ == Status::FETCHING_ACCOUNT_INFO);
  status_ = Status::CONNECTING_TO_USER_PREFS;

  auto pref_registry = pref_connection_delegate_->CreatePrefRegistry();
  RegisterDeviceSyncPrefs(pref_registry.get());

  PA_LOG(INFO) << "DeviceSyncImpl: Connecting to pref service.";
  pref_connection_delegate_->ConnectToPrefService(
      connector_, std::move(pref_registry),
      base::Bind(&DeviceSyncImpl::OnConnectedToPrefService,
                 weak_ptr_factory_.GetWeakPtr()));
}

void DeviceSyncImpl::OnConnectedToPrefService(
    std::unique_ptr<PrefService> pref_service) {
  DCHECK(status_ == Status::CONNECTING_TO_USER_PREFS);
  status_ = Status::WAITING_FOR_ENROLLMENT;

  PA_LOG(INFO) << "DeviceSyncImpl: Connected to pref service; initializing "
               << "CryptAuth managers.";
  pref_service_ = std::move(pref_service);
  InitializeCryptAuthManagementObjects();

  // If enrollment has not yet completed successfully, initialization cannot
  // continue. Once enrollment has finished, OnEnrollmentFinished() is invoked,
  // which finishes the initialization flow.
  if (!cryptauth_enrollment_manager_->IsEnrollmentValid()) {
    PA_LOG(INFO) << "DeviceSyncImpl: Waiting for enrollment to complete.";
    return;
  }

  CompleteInitializationAfterSuccessfulEnrollment();
}

void DeviceSyncImpl::InitializeCryptAuthManagementObjects() {
  DCHECK(status_ == Status::WAITING_FOR_ENROLLMENT);

  // Initialize |cryptauth_gcm_manager_| and have it start listening for GCM
  // tickles.
  cryptauth_gcm_manager_ =
      cryptauth::CryptAuthGCMManagerImpl::Factory::NewInstance(
          gcm_driver_, pref_service_.get());
  cryptauth_gcm_manager_->StartListening();

  cryptauth_client_factory_ = std::make_unique<CryptAuthClientFactoryImpl>(
      identity_manager_, url_request_context_,
      cryptauth::device_classifier_util::GetDeviceClassifier());

  // Initialize |crypauth_device_manager_| and start observing. Start() is not
  // called yet since the device has not completed enrollment.
  cryptauth_device_manager_ =
      cryptauth::CryptAuthDeviceManagerImpl::Factory::NewInstance(
          clock_, cryptauth_client_factory_.get(), cryptauth_gcm_manager_.get(),
          pref_service_.get());

  // Initialize |cryptauth_enrollment_manager_| and start observing, then call
  // Start() immediately to schedule enrollment.
  cryptauth_enrollment_manager_ =
      cryptauth::CryptAuthEnrollmentManagerImpl::Factory::NewInstance(
          clock_,
          std::make_unique<CryptAuthEnrollerFactoryImpl>(
              cryptauth_client_factory_.get()),
          cryptauth::SecureMessageDelegateImpl::Factory::NewInstance(),
          gcm_device_info_provider_->GetGcmDeviceInfo(),
          cryptauth_gcm_manager_.get(), pref_service_.get());
  cryptauth_enrollment_manager_->AddObserver(this);
  cryptauth_enrollment_manager_->Start();
}

void DeviceSyncImpl::CompleteInitializationAfterSuccessfulEnrollment() {
  DCHECK(status_ == Status::WAITING_FOR_ENROLLMENT);
  DCHECK(cryptauth_enrollment_manager_->IsEnrollmentValid());

  // Now that enrollment has completed, the current device has been registered
  // with the CryptAuth back-end and can begin monitoring synced devices.
  cryptauth_device_manager_->Start();

  remote_device_provider_ =
      cryptauth::RemoteDeviceProviderImpl::Factory::NewInstance(
          cryptauth_device_manager_.get(), primary_account_info_.account_id,
          cryptauth_enrollment_manager_->GetUserPrivateKey());
  remote_device_provider_->AddObserver(this);

  software_feature_manager_ =
      cryptauth::SoftwareFeatureManagerImpl::Factory::NewInstance(
          cryptauth_client_factory_.get());

  status_ = Status::READY;

  PA_LOG(INFO) << "DeviceSyncImpl: CryptAuth Enrollment is valid; service "
               << "fully initialized.";
}

base::Optional<cryptauth::RemoteDevice>
DeviceSyncImpl::GetSyncedDeviceWithPublicKey(
    const std::string& public_key) const {
  DCHECK(status_ == Status::READY)
      << "DeviceSyncImpl::GetSyncedDeviceWithPublicKey() called before ready.";

  const auto& synced_devices = remote_device_provider_->GetSyncedDevices();
  const auto& it = std::find_if(synced_devices.begin(), synced_devices.end(),
                                [&public_key](const auto& remote_device) {
                                  return public_key == remote_device.public_key;
                                });

  if (it == synced_devices.end())
    return base::nullopt;

  return *it;
}

void DeviceSyncImpl::OnSetSoftwareFeatureStateSuccess(
    const base::RepeatingCallback<void(const base::Optional<std::string>&)>&
        callback) {
  callback.Run(base::nullopt /* error_code */);
}

void DeviceSyncImpl::OnSetSoftwareFeatureStateError(
    const base::RepeatingCallback<void(const base::Optional<std::string>&)>&
        callback,
    const std::string& error) {
  callback.Run(error);
}

void DeviceSyncImpl::OnFindEligibleDevicesSuccess(
    const base::RepeatingCallback<void(const base::Optional<std::string>&,
                                       mojom::FindEligibleDevicesResponsePtr)>&
        callback,
    const std::vector<cryptauth::ExternalDeviceInfo>& eligible_device_infos,
    const std::vector<cryptauth::IneligibleDevice>& ineligible_devices) {
  std::vector<cryptauth::RemoteDevice> eligible_remote_devices;
  for (const auto& eligible_device_info : eligible_device_infos) {
    auto possible_device =
        GetSyncedDeviceWithPublicKey(eligible_device_info.public_key());
    if (possible_device) {
      eligible_remote_devices.emplace_back(*possible_device);
    } else {
      PA_LOG(ERROR) << "Could not find eligible device with public key \""
                    << eligible_device_info.public_key() << "\".";
    }
  }

  std::vector<cryptauth::RemoteDevice> ineligible_remote_devices;
  for (const auto& ineligible_device : ineligible_devices) {
    auto possible_device =
        GetSyncedDeviceWithPublicKey(ineligible_device.device().public_key());
    if (possible_device) {
      ineligible_remote_devices.emplace_back(*possible_device);
    } else {
      PA_LOG(ERROR) << "Could not find ineligible device with public key \""
                    << ineligible_device.device().public_key() << "\".";
    }
  }

  callback.Run(base::nullopt /* error_code */,
               mojom::FindEligibleDevicesResponse::New(
                   eligible_remote_devices, ineligible_remote_devices));
}

void DeviceSyncImpl::OnFindEligibleDevicesError(
    const base::RepeatingCallback<void(const base::Optional<std::string>&,
                                       mojom::FindEligibleDevicesResponsePtr)>&
        callback,
    const std::string& error) {
  callback.Run(error, nullptr /* response */);
}

void DeviceSyncImpl::SetPrefConnectionDelegateForTesting(
    std::unique_ptr<PrefConnectionDelegate> pref_connection_delegate) {
  pref_connection_delegate_ = std::move(pref_connection_delegate);
}

}  // namespace device_sync

}  // namespace chromeos
