// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/easy_unlock/easy_unlock_service_regular.h"

#include <stdint.h>

#include <utility>

#include "apps/app_lifetime_monitor_factory.h"
#include "base/base64url.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/json/json_string_value_serializer.h"
#include "base/linux_util.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/sys_info.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_clock.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/cryptauth/chrome_cryptauth_service_factory.h"
#include "chrome/browser/chromeos/login/easy_unlock/chrome_proximity_auth_client.h"
#include "chrome/browser/chromeos/login/easy_unlock/easy_unlock_key_manager.h"
#include "chrome/browser/chromeos/login/easy_unlock/easy_unlock_notification_controller.h"
#include "chrome/browser/chromeos/login/easy_unlock/easy_unlock_reauth.h"
#include "chrome/browser/chromeos/login/session/user_session_manager.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/gcm/gcm_profile_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/extensions/api/easy_unlock_private.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/pref_names.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/components/proximity_auth/promotion_manager.h"
#include "chromeos/components/proximity_auth/proximity_auth_pref_names.h"
#include "chromeos/components/proximity_auth/proximity_auth_profile_pref_manager.h"
#include "chromeos/components/proximity_auth/proximity_auth_system.h"
#include "chromeos/components/proximity_auth/screenlock_bridge.h"
#include "chromeos/components/proximity_auth/switches.h"
#include "components/cryptauth/cryptauth_access_token_fetcher.h"
#include "components/cryptauth/cryptauth_client_impl.h"
#include "components/cryptauth/cryptauth_enrollment_manager.h"
#include "components/cryptauth/cryptauth_enrollment_utils.h"
#include "components/cryptauth/cryptauth_gcm_manager_impl.h"
#include "components/cryptauth/local_device_data_provider.h"
#include "components/cryptauth/remote_device_loader.h"
#include "components/cryptauth/secure_message_delegate_impl.h"
#include "components/gcm_driver/gcm_profile_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "components/user_manager/user_manager.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "extensions/browser/event_router.h"
#include "extensions/common/constants.h"
#include "google_apis/gaia/gaia_auth_util.h"

namespace chromeos {

namespace {

// Key name of the local device permit record dictonary in kEasyUnlockPairing.
const char kKeyPermitAccess[] = "permitAccess";

// Key name of the remote device list in kEasyUnlockPairing.
const char kKeyDevices[] = "devices";

enum class SmartLockToggleFeature { DISABLE = false, ENABLE = true };

// The result of a SmartLock operation.
enum class SmartLockResult { FAILURE = false, SUCCESS = true };

enum class SmartLockEnabledState {
  ENABLED = 0,
  DISABLED = 1,
  UNSET = 2,
  COUNT
};

void LogToggleFeature(SmartLockToggleFeature toggle) {
  UMA_HISTOGRAM_BOOLEAN("SmartLock.ToggleFeature", static_cast<bool>(toggle));
}

void LogToggleFeatureDisableResult(SmartLockResult result) {
  UMA_HISTOGRAM_BOOLEAN("SmartLock.ToggleFeature.Disable.Result",
                        static_cast<bool>(result));
}

void LogSmartLockEnabledState(SmartLockEnabledState state) {
  UMA_HISTOGRAM_ENUMERATION("SmartLock.EnabledState", state,
                            SmartLockEnabledState::COUNT);
}

}  // namespace

EasyUnlockServiceRegular::EasyUnlockServiceRegular(Profile* profile)
    : EasyUnlockServiceRegular(
          profile,
          std::make_unique<EasyUnlockNotificationController>(profile)) {}

EasyUnlockServiceRegular::EasyUnlockServiceRegular(
    Profile* profile,
    std::unique_ptr<EasyUnlockNotificationController> notification_controller)
    : EasyUnlockService(profile),
      turn_off_flow_status_(EasyUnlockService::IDLE),
      scoped_crypt_auth_device_manager_observer_(this),
      will_unlock_using_easy_unlock_(false),
      lock_screen_last_shown_timestamp_(base::TimeTicks::Now()),
      deferring_device_load_(false),
      notification_controller_(std::move(notification_controller)),
      shown_pairing_changed_notification_(false),
      weak_ptr_factory_(this) {}

EasyUnlockServiceRegular::~EasyUnlockServiceRegular() {
  registrar_.RemoveAll();
}

// TODO(jhawkins): This method with |has_unlock_keys| == true is the only signal
// that SmartLock setup has completed successfully. Make this signal more
// explicit.
void EasyUnlockServiceRegular::LoadRemoteDevices() {
  bool has_unlock_keys = !GetCryptAuthDeviceManager()->GetUnlockKeys().empty();
  // TODO(jhawkins): The enabled pref should not be tied to whether unlock keys
  // exist; instead, both of these variables should be used to determine
  // IsEnabled().
  pref_manager_->SetIsEasyUnlockEnabled(has_unlock_keys);
  if (has_unlock_keys) {
    // If |has_unlock_keys| is true, then the user must have successfully
    // completed setup. Track that the IsEasyUnlockEnabled pref is actively set
    // by the user, as opposed to passively being set to disabled (the default
    // state).
    pref_manager_->SetEasyUnlockEnabledStateSet();
    LogSmartLockEnabledState(SmartLockEnabledState::ENABLED);
  } else {
    SetProximityAuthDevices(GetAccountId(), cryptauth::RemoteDeviceRefList());

    if (pref_manager_->IsEasyUnlockEnabledStateSet()) {
      LogSmartLockEnabledState(SmartLockEnabledState::DISABLED);
    } else {
      LogSmartLockEnabledState(SmartLockEnabledState::UNSET);
    }
    return;
  }

  // This code path may be hit by:
  //   1. New devices were synced on the lock screen.
  //   2. The service was initialized while the login screen is still up.
  if (proximity_auth::ScreenlockBridge::Get()->IsLocked()) {
    PA_LOG(INFO) << "Deferring device load until screen is unlocked.";
    deferring_device_load_ = true;
    return;
  }

  remote_device_loader_.reset(new cryptauth::RemoteDeviceLoader(
      GetCryptAuthDeviceManager()->GetUnlockKeys(),
      proximity_auth_client()->GetAccountId(),
      GetCryptAuthEnrollmentManager()->GetUserPrivateKey(),
      cryptauth::SecureMessageDelegateImpl::Factory::NewInstance()));
  remote_device_loader_->Load(
      true /* should_load_beacon_seeds */,
      base::Bind(&EasyUnlockServiceRegular::OnRemoteDevicesLoaded,
                 weak_ptr_factory_.GetWeakPtr()));

  // Don't show promotions if EasyUnlock is already enabled.
  promotion_manager_.reset();
}

void EasyUnlockServiceRegular::OnRemoteDevicesLoaded(
    const cryptauth::RemoteDeviceList& remote_devices) {
  cryptauth::RemoteDeviceRefList remote_device_refs;
  for (auto& remote_device : remote_devices) {
    remote_device_refs.push_back(cryptauth::RemoteDeviceRef(
        std::make_shared<cryptauth::RemoteDevice>(remote_device)));
  }

  SetProximityAuthDevices(GetAccountId(), remote_device_refs);

  // We need to store a copy of |remote devices_| in the TPM, so it can be
  // retrieved on the sign-in screen when a user session has not been started
  // yet.
  std::unique_ptr<base::ListValue> device_list(new base::ListValue());
  for (const auto& device : remote_device_refs) {
    std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
    std::string b64_public_key, b64_psk;
    base::Base64UrlEncode(device.public_key(),
                          base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                          &b64_public_key);
    base::Base64UrlEncode(device.persistent_symmetric_key(),
                          base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                          &b64_psk);

    dict->SetString("name", device.name());
    dict->SetString("psk", b64_psk);
    // TODO(jhawkins): Remove the bluetoothAddress field from this proto.
    dict->SetString("bluetoothAddress", std::string());
    dict->SetString("permitId", "permit://google.com/easyunlock/v1/" +
                                    proximity_auth_client()->GetAccountId());
    dict->SetString("permitRecord.id", b64_public_key);
    dict->SetString("permitRecord.type", "license");
    dict->SetString("permitRecord.data", b64_public_key);

    std::unique_ptr<base::ListValue> beacon_seed_list(new base::ListValue());
    for (const auto& beacon_seed : device.beacon_seeds()) {
      std::string b64_beacon_seed;
      base::Base64UrlEncode(beacon_seed.SerializeAsString(),
                            base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                            &b64_beacon_seed);
      beacon_seed_list->AppendString(b64_beacon_seed);
    }

    std::string serialized_beacon_seeds;
    JSONStringValueSerializer serializer(&serialized_beacon_seeds);
    serializer.Serialize(*beacon_seed_list);
    dict->SetString("serializedBeaconSeeds", serialized_beacon_seeds);

    device_list->Append(std::move(dict));
  }

  // TODO(tengs): Rename this function after the easy_unlock app is replaced.
  SetRemoteDevices(*device_list);
}

bool EasyUnlockServiceRegular::ShouldPromote() {
  if (!base::FeatureList::IsEnabled(features::kEasyUnlockPromotions)) {
    return false;
  }

  if (!IsAllowedInternal() || IsEnabled()) {
    return false;
  }

  return true;
}

void EasyUnlockServiceRegular::StartPromotionManager() {
  if (!ShouldPromote() ||
      GetCryptAuthEnrollmentManager()->GetUserPublicKey().empty()) {
    return;
  }

  cryptauth::CryptAuthService* service =
      ChromeCryptAuthServiceFactory::GetInstance()->GetForBrowserContext(
          profile());
  local_device_data_provider_.reset(
      new cryptauth::LocalDeviceDataProvider(service));
  promotion_manager_.reset(new proximity_auth::PromotionManager(
      local_device_data_provider_.get(), notification_controller_.get(),
      pref_manager_.get(), service->CreateCryptAuthClientFactory(),
      base::DefaultClock::GetInstance(), base::ThreadTaskRunnerHandle::Get()));
  promotion_manager_->Start();
}

proximity_auth::ProximityAuthPrefManager*
EasyUnlockServiceRegular::GetProximityAuthPrefManager() {
  return pref_manager_.get();
}

EasyUnlockService::Type EasyUnlockServiceRegular::GetType() const {
  return EasyUnlockService::TYPE_REGULAR;
}

AccountId EasyUnlockServiceRegular::GetAccountId() const {
  const SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfileIfExists(profile());
  // |profile| has to be a signed-in profile with SigninManager already
  // created. Otherwise, just crash to collect stack.
  DCHECK(signin_manager);
  const AccountInfo account_info =
      signin_manager->GetAuthenticatedAccountInfo();
  return account_info.email.empty()
             ? EmptyAccountId()
             : AccountId::FromUserEmailGaiaId(
                   gaia::CanonicalizeEmail(account_info.email),
                   account_info.gaia);
}

void EasyUnlockServiceRegular::LaunchSetup() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  LogToggleFeature(SmartLockToggleFeature::ENABLE);

  // TODO(tengs): To keep login working for existing EasyUnlock users, we need
  // to explicitly disable login here for new users who set up EasyUnlock.
  // After a sufficient number of releases, we should make the default value
  // false.
  pref_manager_->SetIsChromeOSLoginEnabled(false);

  // Force the user to reauthenticate by showing a modal overlay (similar to the
  // lock screen). The password obtained from the reauth is cached for a short
  // period of time and used to create the cryptohome keys for sign-in.
  if (short_lived_user_context_ && short_lived_user_context_->user_context()) {
    OpenSetupApp();
  } else {
    bool reauth_success = EasyUnlockReauth::ReauthForUserContext(
        base::Bind(&EasyUnlockServiceRegular::OpenSetupAppAfterReauth,
                   weak_ptr_factory_.GetWeakPtr()));
    if (!reauth_success)
      OpenSetupApp();
  }
}

void EasyUnlockServiceRegular::HandleUserReauth(
    const UserContext& user_context) {
  // Cache the user context for the next X minutes, so the user doesn't have to
  // reauth again.
  short_lived_user_context_.reset(new ShortLivedUserContext(
      user_context,
      apps::AppLifetimeMonitorFactory::GetForBrowserContext(profile()),
      base::ThreadTaskRunnerHandle::Get().get()));
}

void EasyUnlockServiceRegular::OpenSetupAppAfterReauth(
    const UserContext& user_context) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  HandleUserReauth(user_context);

  OpenSetupApp();

  // Use this opportunity to clear the crytohome keys if it was not already
  // cleared earlier.
  const base::ListValue* devices = GetRemoteDevices();
  if (!devices || devices->empty()) {
    EasyUnlockKeyManager* key_manager =
        UserSessionManager::GetInstance()->GetEasyUnlockKeyManager();
    key_manager->RefreshKeys(
        user_context, base::ListValue(),
        base::Bind(&EasyUnlockServiceRegular::SetHardlockAfterKeyOperation,
                   weak_ptr_factory_.GetWeakPtr(),
                   EasyUnlockScreenlockStateHandler::NO_PAIRING));
  }
}

void EasyUnlockServiceRegular::SetHardlockAfterKeyOperation(
    EasyUnlockScreenlockStateHandler::HardlockState state_on_success,
    bool success) {
  if (success)
    SetHardlockStateForUser(GetAccountId(), state_on_success);

  // Even if the refresh keys operation suceeded, we still fetch and check the
  // cryptohome keys against the keys in local preferences as a sanity check.
  CheckCryptohomeKeysAndMaybeHardlock();
}

void EasyUnlockServiceRegular::ClearPermitAccess() {
  DictionaryPrefUpdate pairing_update(profile()->GetPrefs(),
                                      prefs::kEasyUnlockPairing);
  pairing_update->RemoveWithoutPathExpansion(kKeyPermitAccess, NULL);
}

const base::ListValue* EasyUnlockServiceRegular::GetRemoteDevices() const {
  const base::DictionaryValue* pairing_dict =
      profile()->GetPrefs()->GetDictionary(prefs::kEasyUnlockPairing);
  const base::ListValue* devices = NULL;
  if (pairing_dict && pairing_dict->GetList(kKeyDevices, &devices))
    return devices;
  return NULL;
}

void EasyUnlockServiceRegular::SetRemoteDevices(
    const base::ListValue& devices) {
  std::string remote_devices_json;
  JSONStringValueSerializer serializer(&remote_devices_json);
  serializer.Serialize(devices);
  PA_LOG(INFO) << "Setting RemoteDevices:\n  " << remote_devices_json;

  DictionaryPrefUpdate pairing_update(profile()->GetPrefs(),
                                      prefs::kEasyUnlockPairing);
  if (devices.empty())
    pairing_update->RemoveWithoutPathExpansion(kKeyDevices, NULL);
  else
    pairing_update->SetKey(kKeyDevices, devices.Clone());

  RefreshCryptohomeKeysIfPossible();
}

void EasyUnlockServiceRegular::RunTurnOffFlow() {
  if (turn_off_flow_status_ == PENDING)
    return;
  DCHECK(!cryptauth_client_);

  LogToggleFeature(SmartLockToggleFeature::DISABLE);

  SetTurnOffFlowStatus(PENDING);

  std::unique_ptr<cryptauth::CryptAuthClientFactory> factory =
      proximity_auth_client()->CreateCryptAuthClientFactory();
  cryptauth_client_ = factory->CreateInstance();

  cryptauth::ToggleEasyUnlockRequest request;
  request.set_enable(false);
  request.set_apply_to_all(true);
  cryptauth_client_->ToggleEasyUnlock(
      request,
      base::Bind(&EasyUnlockServiceRegular::OnToggleEasyUnlockApiComplete,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&EasyUnlockServiceRegular::OnToggleEasyUnlockApiFailed,
                 weak_ptr_factory_.GetWeakPtr()));
}

void EasyUnlockServiceRegular::ResetTurnOffFlow() {
  cryptauth_client_.reset();
  SetTurnOffFlowStatus(IDLE);
}

EasyUnlockService::TurnOffFlowStatus
EasyUnlockServiceRegular::GetTurnOffFlowStatus() const {
  return turn_off_flow_status_;
}

std::string EasyUnlockServiceRegular::GetChallenge() const {
  return std::string();
}

std::string EasyUnlockServiceRegular::GetWrappedSecret() const {
  return std::string();
}

void EasyUnlockServiceRegular::RecordEasySignInOutcome(
    const AccountId& account_id,
    bool success) const {
  NOTREACHED();
}

void EasyUnlockServiceRegular::RecordPasswordLoginEvent(
    const AccountId& account_id) const {
  NOTREACHED();
}

void EasyUnlockServiceRegular::InitializeInternal() {
  proximity_auth::ScreenlockBridge::Get()->AddObserver(this);

  pref_manager_.reset(new proximity_auth::ProximityAuthProfilePrefManager(
      profile()->GetPrefs()));

  // TODO(tengs): Due to badly configured browser_tests, Chrome crashes during
  // shutdown. Revisit this condition after migration is fully completed.
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kTestType)) {
    // Note: There is no local state in tests.
    if (g_browser_process->local_state()) {
      pref_manager_->StartSyncingToLocalState(g_browser_process->local_state(),
                                              GetAccountId());
    }

    scoped_crypt_auth_device_manager_observer_.Add(GetCryptAuthDeviceManager());
    LoadRemoteDevices();
    StartPromotionManager();
  }

  registrar_.Init(profile()->GetPrefs());
  registrar_.Add(
      proximity_auth::prefs::kProximityAuthIsChromeOSLoginEnabled,
      base::Bind(&EasyUnlockServiceRegular::RefreshCryptohomeKeysIfPossible,
                 weak_ptr_factory_.GetWeakPtr()));
}

void EasyUnlockServiceRegular::ShutdownInternal() {
  short_lived_user_context_.reset();

  turn_off_flow_status_ = EasyUnlockService::IDLE;
  proximity_auth::ScreenlockBridge::Get()->RemoveObserver(this);
  scoped_crypt_auth_device_manager_observer_.RemoveAll();
}

bool EasyUnlockServiceRegular::IsAllowedInternal() const {
  user_manager::UserManager* user_manager = user_manager::UserManager::Get();
  if (!user_manager->IsLoggedInAsUserWithGaiaAccount())
    return false;

  // TODO(tengs): Ephemeral accounts generate a new enrollment every time they
  // are added, so disable Smart Lock to reduce enrollments on server. However,
  // ephemeral accounts can be locked, so we should revisit this use case.
  if (user_manager->IsCurrentUserNonCryptohomeDataEphemeral())
    return false;

  if (!ProfileHelper::IsPrimaryProfile(profile()))
    return false;

  if (!profile()->GetPrefs()->GetBoolean(prefs::kEasyUnlockAllowed))
    return false;

  return true;
}

bool EasyUnlockServiceRegular::IsEnabled() const {
  return pref_manager_ && pref_manager_->IsEasyUnlockEnabled();
}

bool EasyUnlockServiceRegular::IsChromeOSLoginEnabled() const {
  return pref_manager_ && pref_manager_->IsChromeOSLoginEnabled();
}

void EasyUnlockServiceRegular::OnWillFinalizeUnlock(bool success) {
  will_unlock_using_easy_unlock_ = success;
}

void EasyUnlockServiceRegular::OnSuspendDoneInternal() {
  lock_screen_last_shown_timestamp_ = base::TimeTicks::Now();
}

void EasyUnlockServiceRegular::OnSyncStarted() {
  unlock_keys_before_sync_ = GetCryptAuthDeviceManager()->GetUnlockKeys();
}

void EasyUnlockServiceRegular::OnSyncFinished(
    cryptauth::CryptAuthDeviceManager::SyncResult sync_result,
    cryptauth::CryptAuthDeviceManager::DeviceChangeResult
        device_change_result) {
  if (sync_result == cryptauth::CryptAuthDeviceManager::SyncResult::FAILURE)
    return;

  std::set<std::string> public_keys_before_sync;
  for (const auto& device_info : unlock_keys_before_sync_) {
    public_keys_before_sync.insert(device_info.public_key());
  }
  unlock_keys_before_sync_.clear();

  std::vector<cryptauth::ExternalDeviceInfo> unlock_keys_after_sync =
      GetCryptAuthDeviceManager()->GetUnlockKeys();
  std::set<std::string> public_keys_after_sync;
  for (const auto& device_info : unlock_keys_after_sync) {
    public_keys_after_sync.insert(device_info.public_key());
  }

  if (public_keys_after_sync.empty())
    ClearPermitAccess();

  if (public_keys_before_sync == public_keys_after_sync)
    return;

  // Show the appropriate notification if an unlock key is first synced or if it
  // changes an existing key.
  // Note: We do not show a notification when EasyUnlock is disabled by sync nor
  // if EasyUnlock was enabled through the setup app.
  bool is_setup_fresh =
      short_lived_user_context_ && short_lived_user_context_->user_context();

  if (public_keys_after_sync.size() > 0 && !is_setup_fresh) {
    if (public_keys_before_sync.size() == 0) {
      notification_controller_->ShowChromebookAddedNotification();
    } else {
      shown_pairing_changed_notification_ = true;
      notification_controller_->ShowPairingChangeNotification();
    }
  }

  // The enrollment has finished when the sync is finished.
  StartPromotionManager();

  LoadRemoteDevices();
}

void EasyUnlockServiceRegular::OnScreenDidLock(
    proximity_auth::ScreenlockBridge::LockHandler::ScreenType screen_type) {
  will_unlock_using_easy_unlock_ = false;
  lock_screen_last_shown_timestamp_ = base::TimeTicks::Now();
}

void EasyUnlockServiceRegular::OnScreenDidUnlock(
    proximity_auth::ScreenlockBridge::LockHandler::ScreenType screen_type) {
  if (!will_unlock_using_easy_unlock_ && pref_manager_ &&
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          proximity_auth::switches::kEnableForcePasswordReauth)) {
    // If a password was used, then record the current timestamp. This timestamp
    // is used to enforce password reauths after a certain time has elapsed.
    // Note: This code path is also triggered by the login flow.
    pref_manager_->SetLastPasswordEntryTimestampMs(
        base::Time::Now().ToJavaTime());
  }

  // If we tried to load remote devices (e.g. after a sync or the
  // service was initialized) while the screen was locked, we can now
  // load the new remote devices.
  //
  // It's important to go through this code path even if unlocking the
  // login screen. Because when the service is initialized while the
  // user is signing in we need to load the remotes. Otherwise, the
  // first time the user locks the screen the feature won't work.
  if (deferring_device_load_) {
    PA_LOG(INFO) << "Loading deferred devices after screen unlock.";
    deferring_device_load_ = false;
    LoadRemoteDevices();
  }

  // Do not process events for the login screen.
  if (screen_type != proximity_auth::ScreenlockBridge::LockHandler::LOCK_SCREEN)
    return;

  if (shown_pairing_changed_notification_) {
    shown_pairing_changed_notification_ = false;
    std::vector<cryptauth::ExternalDeviceInfo> unlock_keys =
        GetCryptAuthDeviceManager()->GetUnlockKeys();
    if (!unlock_keys.empty()) {
      // TODO(tengs): Right now, we assume that there is only one possible
      // unlock key. We need to update this notification be more generic.
      notification_controller_->ShowPairingChangeAppliedNotification(
          unlock_keys[0].friendly_device_name());
    }
  }

  // Only record metrics for users who have enabled the feature.
  if (IsEnabled()) {
    EasyUnlockAuthEvent event = will_unlock_using_easy_unlock_
                                    ? EASY_UNLOCK_SUCCESS
                                    : GetPasswordAuthEvent();
    RecordEasyUnlockScreenUnlockEvent(event);

    if (will_unlock_using_easy_unlock_) {
      RecordEasyUnlockScreenUnlockDuration(base::TimeTicks::Now() -
                                           lock_screen_last_shown_timestamp_);
    }
  }

  will_unlock_using_easy_unlock_ = false;
}

void EasyUnlockServiceRegular::OnFocusedUserChanged(
    const AccountId& account_id) {
  // Nothing to do.
}

void EasyUnlockServiceRegular::SetTurnOffFlowStatus(TurnOffFlowStatus status) {
  turn_off_flow_status_ = status;
  NotifyTurnOffOperationStatusChanged();
}

void EasyUnlockServiceRegular::OnToggleEasyUnlockApiComplete(
    const cryptauth::ToggleEasyUnlockResponse& response) {
  LogToggleFeatureDisableResult(SmartLockResult::SUCCESS);

  cryptauth_client_.reset();

  GetCryptAuthDeviceManager()->ForceSyncNow(
      cryptauth::InvocationReason::INVOCATION_REASON_FEATURE_TOGGLED);
  EasyUnlockService::ResetLocalStateForUser(GetAccountId());
  SetRemoteDevices(base::ListValue());
  SetProximityAuthDevices(GetAccountId(), cryptauth::RemoteDeviceRefList());
  pref_manager_->SetIsEasyUnlockEnabled(false);
  SetTurnOffFlowStatus(IDLE);
  pref_manager_->SetIsEasyUnlockEnabled(false);
  ResetScreenlockState();
  registrar_.RemoveAll();
}

void EasyUnlockServiceRegular::OnToggleEasyUnlockApiFailed(
    const std::string& error_message) {
  LOG(WARNING) << "Failed to turn off Smart Lock: " << error_message;
  LogToggleFeatureDisableResult(SmartLockResult::FAILURE);
  SetTurnOffFlowStatus(FAIL);
}

cryptauth::CryptAuthEnrollmentManager*
EasyUnlockServiceRegular::GetCryptAuthEnrollmentManager() {
  cryptauth::CryptAuthEnrollmentManager* manager =
      ChromeCryptAuthServiceFactory::GetInstance()
          ->GetForBrowserContext(profile())
          ->GetCryptAuthEnrollmentManager();
  DCHECK(manager);
  return manager;
}

cryptauth::CryptAuthDeviceManager*
EasyUnlockServiceRegular::GetCryptAuthDeviceManager() {
  cryptauth::CryptAuthDeviceManager* manager =
      ChromeCryptAuthServiceFactory::GetInstance()
          ->GetForBrowserContext(profile())
          ->GetCryptAuthDeviceManager();
  DCHECK(manager);
  return manager;
}

void EasyUnlockServiceRegular::RefreshCryptohomeKeysIfPossible() {
  // If the user reauthed on the settings page, then the UserContext will be
  // cached.
  if (short_lived_user_context_ && short_lived_user_context_->user_context()) {
    // We only sync the remote devices to cryptohome if the user has enabled
    // EasyUnlock on the login screen.
    base::ListValue empty_list;
    const base::ListValue* remote_devices_list = GetRemoteDevices();
    if (!IsChromeOSLoginEnabled() || !remote_devices_list)
      remote_devices_list = &empty_list;

    UserSessionManager::GetInstance()->GetEasyUnlockKeyManager()->RefreshKeys(
        *short_lived_user_context_->user_context(),
        base::ListValue(remote_devices_list->GetList()),
        base::Bind(&EasyUnlockServiceRegular::SetHardlockAfterKeyOperation,
                   weak_ptr_factory_.GetWeakPtr(),
                   EasyUnlockScreenlockStateHandler::NO_HARDLOCK));
  } else {
    CheckCryptohomeKeysAndMaybeHardlock();
  }
}

}  // namespace chromeos
