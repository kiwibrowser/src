// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/proximity_auth/webui/proximity_auth_webui_handler.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/base64url.h"
#include "base/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_clock.h"
#include "base/time/default_tick_clock.h"
#include "base/values.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/components/proximity_auth/messenger.h"
#include "chromeos/components/proximity_auth/remote_device_life_cycle_impl.h"
#include "chromeos/components/proximity_auth/remote_status_update.h"
#include "chromeos/components/proximity_auth/webui/reachable_phone_flow.h"
#include "components/cryptauth/cryptauth_enrollment_manager.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device_loader.h"
#include "components/cryptauth/remote_device_ref.h"
#include "components/cryptauth/secure_context.h"
#include "components/cryptauth/secure_message_delegate_impl.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_ui.h"
#include "device/bluetooth/bluetooth_uuid.h"

namespace proximity_auth {

namespace {

// Keys in the JSON representation of a log message.
const char kLogMessageTextKey[] = "text";
const char kLogMessageTimeKey[] = "time";
const char kLogMessageFileKey[] = "file";
const char kLogMessageLineKey[] = "line";
const char kLogMessageSeverityKey[] = "severity";

// Keys in the JSON representation of a SyncState object for enrollment or
// device sync.
const char kSyncStateLastSuccessTime[] = "lastSuccessTime";
const char kSyncStateNextRefreshTime[] = "nextRefreshTime";
const char kSyncStateRecoveringFromFailure[] = "recoveringFromFailure";
const char kSyncStateOperationInProgress[] = "operationInProgress";

// Converts |log_message| to a raw dictionary value used as a JSON argument to
// JavaScript functions.
std::unique_ptr<base::DictionaryValue> LogMessageToDictionary(
    const LogBuffer::LogMessage& log_message) {
  std::unique_ptr<base::DictionaryValue> dictionary(
      new base::DictionaryValue());
  dictionary->SetString(kLogMessageTextKey, log_message.text);
  dictionary->SetString(
      kLogMessageTimeKey,
      base::TimeFormatTimeOfDayWithMilliseconds(log_message.time));
  dictionary->SetString(kLogMessageFileKey, log_message.file);
  dictionary->SetInteger(kLogMessageLineKey, log_message.line);
  dictionary->SetInteger(kLogMessageSeverityKey,
                         static_cast<int>(log_message.severity));
  return dictionary;
}

// Keys in the JSON representation of an ExternalDeviceInfo proto.
const char kExternalDevicePublicKey[] = "publicKey";
const char kExternalDevicePublicKeyTruncated[] = "publicKeyTruncated";
const char kExternalDeviceFriendlyName[] = "friendlyDeviceName";
const char kExternalDeviceBluetoothAddress[] = "bluetoothAddress";
const char kExternalDeviceUnlockKey[] = "unlockKey";
const char kExternalDeviceMobileHotspot[] = "hasMobileHotspot";
const char kExternalDeviceIsArcPlusPlusEnrollment[] = "isArcPlusPlusEnrollment";
const char kExternalDeviceIsPixelPhone[] = "isPixelPhone";
const char kExternalDeviceConnectionStatus[] = "connectionStatus";
const char kExternalDeviceRemoteState[] = "remoteState";

// The possible values of the |kExternalDeviceConnectionStatus| field.
const char kExternalDeviceConnected[] = "connected";
const char kExternalDeviceDisconnected[] = "disconnected";
const char kExternalDeviceConnecting[] = "connecting";

// Keys in the JSON representation of an IneligibleDevice proto.
const char kIneligibleDeviceReasons[] = "ineligibilityReasons";

// Creates a SyncState JSON object that can be passed to the WebUI.
std::unique_ptr<base::DictionaryValue> CreateSyncStateDictionary(
    double last_success_time,
    double next_refresh_time,
    bool is_recovering_from_failure,
    bool is_enrollment_in_progress) {
  std::unique_ptr<base::DictionaryValue> sync_state(
      new base::DictionaryValue());
  sync_state->SetDouble(kSyncStateLastSuccessTime, last_success_time);
  sync_state->SetDouble(kSyncStateNextRefreshTime, next_refresh_time);
  sync_state->SetBoolean(kSyncStateRecoveringFromFailure,
                         is_recovering_from_failure);
  sync_state->SetBoolean(kSyncStateOperationInProgress,
                         is_enrollment_in_progress);
  return sync_state;
}

}  // namespace

ProximityAuthWebUIHandler::ProximityAuthWebUIHandler(
    ProximityAuthClient* proximity_auth_client)
    : proximity_auth_client_(proximity_auth_client),
      web_contents_initialized_(false),
      weak_ptr_factory_(this) {
  cryptauth_client_factory_ =
      proximity_auth_client_->CreateCryptAuthClientFactory();
}

ProximityAuthWebUIHandler::~ProximityAuthWebUIHandler() {
  LogBuffer::GetInstance()->RemoveObserver(this);
  cryptauth::CryptAuthDeviceManager* device_manager =
      proximity_auth_client_->GetCryptAuthDeviceManager();
  if (device_manager)
    device_manager->RemoveObserver(this);
}

void ProximityAuthWebUIHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "onWebContentsInitialized",
      base::BindRepeating(&ProximityAuthWebUIHandler::OnWebContentsInitialized,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "clearLogBuffer",
      base::BindRepeating(&ProximityAuthWebUIHandler::ClearLogBuffer,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getLogMessages",
      base::BindRepeating(&ProximityAuthWebUIHandler::GetLogMessages,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "toggleUnlockKey",
      base::BindRepeating(&ProximityAuthWebUIHandler::ToggleUnlockKey,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "findEligibleUnlockDevices",
      base::BindRepeating(&ProximityAuthWebUIHandler::FindEligibleUnlockDevices,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "findReachableDevices",
      base::BindRepeating(&ProximityAuthWebUIHandler::FindReachableDevices,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getLocalState",
      base::BindRepeating(&ProximityAuthWebUIHandler::GetLocalState,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "forceEnrollment",
      base::BindRepeating(&ProximityAuthWebUIHandler::ForceEnrollment,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "forceDeviceSync",
      base::BindRepeating(&ProximityAuthWebUIHandler::ForceDeviceSync,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "toggleConnection",
      base::BindRepeating(&ProximityAuthWebUIHandler::ToggleConnection,
                          base::Unretained(this)));
}

void ProximityAuthWebUIHandler::OnLogMessageAdded(
    const LogBuffer::LogMessage& log_message) {
  std::unique_ptr<base::DictionaryValue> dictionary =
      LogMessageToDictionary(log_message);
  web_ui()->CallJavascriptFunctionUnsafe("LogBufferInterface.onLogMessageAdded",
                                         *dictionary);
}

void ProximityAuthWebUIHandler::OnLogBufferCleared() {
  web_ui()->CallJavascriptFunctionUnsafe(
      "LogBufferInterface.onLogBufferCleared");
}

void ProximityAuthWebUIHandler::OnEnrollmentStarted() {
  web_ui()->CallJavascriptFunctionUnsafe(
      "LocalStateInterface.onEnrollmentStateChanged",
      *GetEnrollmentStateDictionary());
}

void ProximityAuthWebUIHandler::OnEnrollmentFinished(bool success) {
  std::unique_ptr<base::DictionaryValue> enrollment_state =
      GetEnrollmentStateDictionary();
  PA_LOG(INFO) << "Enrollment attempt completed with success=" << success
               << ":\n"
               << *enrollment_state;
  web_ui()->CallJavascriptFunctionUnsafe(
      "LocalStateInterface.onEnrollmentStateChanged", *enrollment_state);
}

void ProximityAuthWebUIHandler::OnSyncStarted() {
  web_ui()->CallJavascriptFunctionUnsafe(
      "LocalStateInterface.onDeviceSyncStateChanged",
      *GetDeviceSyncStateDictionary());
}

void ProximityAuthWebUIHandler::OnSyncFinished(
    cryptauth::CryptAuthDeviceManager::SyncResult sync_result,
    cryptauth::CryptAuthDeviceManager::DeviceChangeResult
        device_change_result) {
  std::unique_ptr<base::DictionaryValue> device_sync_state =
      GetDeviceSyncStateDictionary();
  PA_LOG(INFO) << "Device sync completed with result="
               << static_cast<int>(sync_result) << ":\n"
               << *device_sync_state;
  web_ui()->CallJavascriptFunctionUnsafe(
      "LocalStateInterface.onDeviceSyncStateChanged", *device_sync_state);

  if (device_change_result ==
      cryptauth::CryptAuthDeviceManager::DeviceChangeResult::CHANGED) {
    std::unique_ptr<base::ListValue> synced_devices = GetRemoteDevicesList();
    PA_LOG(INFO) << "New unlock keys obtained after device sync:\n"
                 << *synced_devices;
    web_ui()->CallJavascriptFunctionUnsafe(
        "LocalStateInterface.onRemoteDevicesChanged", *synced_devices);
  }
}

void ProximityAuthWebUIHandler::OnWebContentsInitialized(
    const base::ListValue* args) {
  if (!web_contents_initialized_) {
    cryptauth::CryptAuthEnrollmentManager* enrollment_manager =
        proximity_auth_client_->GetCryptAuthEnrollmentManager();
    if (enrollment_manager)
      enrollment_manager->AddObserver(this);

    cryptauth::CryptAuthDeviceManager* device_manager =
        proximity_auth_client_->GetCryptAuthDeviceManager();
    if (device_manager)
      device_manager->AddObserver(this);

    LogBuffer::GetInstance()->AddObserver(this);

    web_contents_initialized_ = true;
  }
}

void ProximityAuthWebUIHandler::GetLogMessages(const base::ListValue* args) {
  base::ListValue json_logs;
  for (const auto& log : *LogBuffer::GetInstance()->logs()) {
    json_logs.Append(LogMessageToDictionary(log));
  }
  web_ui()->CallJavascriptFunctionUnsafe("LogBufferInterface.onGotLogMessages",
                                         json_logs);
}

void ProximityAuthWebUIHandler::ClearLogBuffer(const base::ListValue* args) {
  // The OnLogBufferCleared() observer function will be called after the buffer
  // is cleared.
  LogBuffer::GetInstance()->Clear();
}

void ProximityAuthWebUIHandler::ToggleUnlockKey(const base::ListValue* args) {
  std::string public_key_b64, public_key;
  bool make_unlock_key;
  if (args->GetSize() != 2 || !args->GetString(0, &public_key_b64) ||
      !args->GetBoolean(1, &make_unlock_key) ||
      !base::Base64UrlDecode(public_key_b64,
                             base::Base64UrlDecodePolicy::REQUIRE_PADDING,
                             &public_key)) {
    PA_LOG(ERROR) << "Invalid arguments to toggleUnlockKey";
    return;
  }

  cryptauth::ToggleEasyUnlockRequest request;
  request.set_enable(make_unlock_key);
  request.set_public_key(public_key);
  *(request.mutable_device_classifier()) =
      proximity_auth_client_->GetDeviceClassifier();

  PA_LOG(INFO) << "Toggling unlock key:\n"
               << "    public_key: " << public_key_b64 << "\n"
               << "    make_unlock_key: " << make_unlock_key;
  cryptauth_client_ = cryptauth_client_factory_->CreateInstance();
  cryptauth_client_->ToggleEasyUnlock(
      request,
      base::Bind(&ProximityAuthWebUIHandler::OnEasyUnlockToggled,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&ProximityAuthWebUIHandler::OnCryptAuthClientError,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ProximityAuthWebUIHandler::FindEligibleUnlockDevices(
    const base::ListValue* args) {
  cryptauth_client_ = cryptauth_client_factory_->CreateInstance();

  cryptauth::FindEligibleUnlockDevicesRequest request;
  *(request.mutable_device_classifier()) =
      proximity_auth_client_->GetDeviceClassifier();
  cryptauth_client_->FindEligibleUnlockDevices(
      request,
      base::Bind(&ProximityAuthWebUIHandler::OnFoundEligibleUnlockDevices,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&ProximityAuthWebUIHandler::OnCryptAuthClientError,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ProximityAuthWebUIHandler::FindReachableDevices(
    const base::ListValue* args) {
  if (reachable_phone_flow_) {
    PA_LOG(INFO) << "Waiting for existing ReachablePhoneFlow to finish.";
    return;
  }

  reachable_phone_flow_.reset(
      new ReachablePhoneFlow(cryptauth_client_factory_.get()));
  reachable_phone_flow_->Run(
      base::Bind(&ProximityAuthWebUIHandler::OnReachablePhonesFound,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ProximityAuthWebUIHandler::ForceEnrollment(const base::ListValue* args) {
  cryptauth::CryptAuthEnrollmentManager* enrollment_manager =
      proximity_auth_client_->GetCryptAuthEnrollmentManager();
  if (enrollment_manager) {
    enrollment_manager->ForceEnrollmentNow(cryptauth::INVOCATION_REASON_MANUAL);
  }
}

void ProximityAuthWebUIHandler::ForceDeviceSync(const base::ListValue* args) {
  cryptauth::CryptAuthDeviceManager* device_manager =
      proximity_auth_client_->GetCryptAuthDeviceManager();
  if (device_manager)
    device_manager->ForceSyncNow(cryptauth::INVOCATION_REASON_MANUAL);
}

void ProximityAuthWebUIHandler::ToggleConnection(const base::ListValue* args) {
  cryptauth::CryptAuthEnrollmentManager* enrollment_manager =
      proximity_auth_client_->GetCryptAuthEnrollmentManager();
  cryptauth::CryptAuthDeviceManager* device_manager =
      proximity_auth_client_->GetCryptAuthDeviceManager();
  if (!enrollment_manager || !device_manager)
    return;

  std::string b64_public_key;
  std::string public_key;
  if (!enrollment_manager || !device_manager || !args->GetSize() ||
      !args->GetString(0, &b64_public_key) ||
      !base::Base64UrlDecode(b64_public_key,
                             base::Base64UrlDecodePolicy::REQUIRE_PADDING,
                             &public_key)) {
    return;
  }

  std::string selected_device_public_key;
  if (selected_remote_device_)
    selected_device_public_key = selected_remote_device_->public_key();

  for (const auto& unlock_key : device_manager->GetUnlockKeys()) {
    if (unlock_key.public_key() == public_key) {
      if (life_cycle_ && selected_device_public_key == public_key) {
        CleanUpRemoteDeviceLifeCycle();
        return;
      }

      // TODO(sacomoto): Pass an instance of ProximityAuthPrefManager. This is
      // used to get the address of BLE devices.
      remote_device_loader_.reset(new cryptauth::RemoteDeviceLoader(
          std::vector<cryptauth::ExternalDeviceInfo>(1, unlock_key),
          proximity_auth_client_->GetAccountId(),
          enrollment_manager->GetUserPrivateKey(),
          cryptauth::SecureMessageDelegateImpl::Factory::NewInstance()));
      remote_device_loader_->Load(
          true /* should_load_beacon_seeds */,
          base::Bind(&ProximityAuthWebUIHandler::OnRemoteDevicesLoaded,
                     weak_ptr_factory_.GetWeakPtr()));
      return;
    }
  }

  PA_LOG(ERROR) << "Unlock key (" << b64_public_key << ") not found";
}

void ProximityAuthWebUIHandler::OnCryptAuthClientError(
    const std::string& error_message) {
  PA_LOG(WARNING) << "CryptAuth request failed: " << error_message;
  base::Value error_string(error_message);
  web_ui()->CallJavascriptFunctionUnsafe("CryptAuthInterface.onError",
                                         error_string);
}

void ProximityAuthWebUIHandler::OnEasyUnlockToggled(
    const cryptauth::ToggleEasyUnlockResponse& response) {
  web_ui()->CallJavascriptFunctionUnsafe(
      "CryptAuthInterface.onUnlockKeyToggled");
  // TODO(tengs): Update the local state to reflect the toggle.
}

void ProximityAuthWebUIHandler::OnFoundEligibleUnlockDevices(
    const cryptauth::FindEligibleUnlockDevicesResponse& response) {
  base::ListValue eligible_devices;
  for (const auto& external_device : response.eligible_devices()) {
    eligible_devices.Append(ExternalDeviceInfoToDictionary(external_device));
  }

  base::ListValue ineligible_devices;
  for (const auto& ineligible_device : response.ineligible_devices()) {
    ineligible_devices.Append(IneligibleDeviceToDictionary(ineligible_device));
  }

  PA_LOG(INFO) << "Found " << eligible_devices.GetSize()
               << " eligible devices and " << ineligible_devices.GetSize()
               << " ineligible devices.";
  web_ui()->CallJavascriptFunctionUnsafe(
      "CryptAuthInterface.onGotEligibleDevices", eligible_devices,
      ineligible_devices);
}

void ProximityAuthWebUIHandler::OnReachablePhonesFound(
    const std::vector<cryptauth::ExternalDeviceInfo>& reachable_phones) {
  reachable_phone_flow_.reset();
  base::ListValue device_list;
  for (const auto& external_device : reachable_phones) {
    device_list.Append(ExternalDeviceInfoToDictionary(external_device));
  }
  web_ui()->CallJavascriptFunctionUnsafe(
      "CryptAuthInterface.onGotReachableDevices", device_list);
}

void ProximityAuthWebUIHandler::GetLocalState(const base::ListValue* args) {
  std::unique_ptr<base::Value> truncated_local_device_id =
      GetTruncatedLocalDeviceId();
  std::unique_ptr<base::DictionaryValue> enrollment_state =
      GetEnrollmentStateDictionary();
  std::unique_ptr<base::DictionaryValue> device_sync_state =
      GetDeviceSyncStateDictionary();
  std::unique_ptr<base::ListValue> synced_devices = GetRemoteDevicesList();

  PA_LOG(INFO) << "==== Got Local State ====\n"
               << "Device ID (truncated): " << *truncated_local_device_id
               << "\nEnrollment State: \n"
               << *enrollment_state << "Device Sync State: \n"
               << *device_sync_state << "Unlock Keys: \n"
               << *synced_devices;
  web_ui()->CallJavascriptFunctionUnsafe(
      "LocalStateInterface.onGotLocalState", *truncated_local_device_id,
      *enrollment_state, *device_sync_state, *synced_devices);
}

std::unique_ptr<base::Value>
ProximityAuthWebUIHandler::GetTruncatedLocalDeviceId() {
  std::string local_public_key =
      proximity_auth_client_->GetLocalDevicePublicKey();

  std::string device_id;
  base::Base64UrlEncode(local_public_key,
                        base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                        &device_id);

  return std::make_unique<base::Value>(
      cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(device_id));
}

std::unique_ptr<base::DictionaryValue>
ProximityAuthWebUIHandler::GetEnrollmentStateDictionary() {
  cryptauth::CryptAuthEnrollmentManager* enrollment_manager =
      proximity_auth_client_->GetCryptAuthEnrollmentManager();
  if (!enrollment_manager)
    return std::make_unique<base::DictionaryValue>();

  return CreateSyncStateDictionary(
      enrollment_manager->GetLastEnrollmentTime().ToJsTime(),
      enrollment_manager->GetTimeToNextAttempt().InMillisecondsF(),
      enrollment_manager->IsRecoveringFromFailure(),
      enrollment_manager->IsEnrollmentInProgress());
}

std::unique_ptr<base::DictionaryValue>
ProximityAuthWebUIHandler::GetDeviceSyncStateDictionary() {
  cryptauth::CryptAuthDeviceManager* device_manager =
      proximity_auth_client_->GetCryptAuthDeviceManager();
  if (!device_manager)
    return std::make_unique<base::DictionaryValue>();

  return CreateSyncStateDictionary(
      device_manager->GetLastSyncTime().ToJsTime(),
      device_manager->GetTimeToNextAttempt().InMillisecondsF(),
      device_manager->IsRecoveringFromFailure(),
      device_manager->IsSyncInProgress());
}

std::unique_ptr<base::ListValue>
ProximityAuthWebUIHandler::GetRemoteDevicesList() {
  std::unique_ptr<base::ListValue> unlock_keys(new base::ListValue());
  cryptauth::CryptAuthDeviceManager* device_manager =
      proximity_auth_client_->GetCryptAuthDeviceManager();
  if (!device_manager)
    return unlock_keys;

  for (const auto& unlock_key : device_manager->GetSyncedDevices()) {
    unlock_keys->Append(ExternalDeviceInfoToDictionary(unlock_key));
  }

  return unlock_keys;
}

void ProximityAuthWebUIHandler::OnRemoteDevicesLoaded(
    const cryptauth::RemoteDeviceList& remote_devices) {
  if (remote_devices.empty()) {
    PA_LOG(WARNING) << "Remote device list is empty.";
    return;
  }

  if (remote_devices[0].persistent_symmetric_key.empty()) {
    PA_LOG(ERROR) << "Failed to derive PSK.";
    return;
  }

  selected_remote_device_ = cryptauth::RemoteDeviceRef(
      std::make_shared<cryptauth::RemoteDevice>(remote_devices[0]));
  life_cycle_.reset(new RemoteDeviceLifeCycleImpl(*selected_remote_device_));
  life_cycle_->AddObserver(this);
  life_cycle_->Start();
}

std::unique_ptr<base::DictionaryValue>
ProximityAuthWebUIHandler::ExternalDeviceInfoToDictionary(
    const cryptauth::ExternalDeviceInfo& device_info) {
  std::string base64_public_key;
  base::Base64UrlEncode(device_info.public_key(),
                        base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                        &base64_public_key);

  // Set the fields in the ExternalDeviceInfo proto.
  std::unique_ptr<base::DictionaryValue> dictionary(
      new base::DictionaryValue());
  dictionary->SetString(kExternalDevicePublicKey, base64_public_key);
  dictionary->SetString(
      kExternalDevicePublicKeyTruncated,
      cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(base64_public_key));
  dictionary->SetString(kExternalDeviceFriendlyName,
                        device_info.friendly_device_name());
  dictionary->SetString(kExternalDeviceBluetoothAddress,
                        device_info.bluetooth_address());
  dictionary->SetBoolean(kExternalDeviceUnlockKey, device_info.unlock_key());
  dictionary->SetBoolean(kExternalDeviceMobileHotspot,
                         device_info.mobile_hotspot_supported());
  dictionary->SetBoolean(kExternalDeviceIsArcPlusPlusEnrollment,
                         device_info.arc_plus_plus());
  dictionary->SetBoolean(kExternalDeviceIsPixelPhone,
                         device_info.pixel_phone());
  dictionary->SetString(kExternalDeviceConnectionStatus,
                        kExternalDeviceDisconnected);

  cryptauth::CryptAuthDeviceManager* device_manager =
      proximity_auth_client_->GetCryptAuthDeviceManager();
  if (!device_manager)
    return dictionary;

  // If |device_info| is a known unlock key, then combine the proto data with
  // the corresponding local device data (e.g. connection status and remote
  // status updates).
  std::string public_key = device_info.public_key();
  std::vector<cryptauth::ExternalDeviceInfo> unlock_keys =
      device_manager->GetUnlockKeys();
  auto iterator = std::find_if(
      unlock_keys.begin(), unlock_keys.end(),
      [&public_key](const cryptauth::ExternalDeviceInfo& unlock_key) {
        return unlock_key.public_key() == public_key;
      });

  std::string selected_device_public_key;
  if (selected_remote_device_)
    selected_device_public_key = selected_remote_device_->public_key();

  if (iterator == unlock_keys.end() ||
      selected_device_public_key != device_info.public_key())
    return dictionary;

  // Fill in the current Bluetooth connection status.
  std::string connection_status = kExternalDeviceDisconnected;
  if (life_cycle_ &&
      life_cycle_->GetState() ==
          RemoteDeviceLifeCycle::State::SECURE_CHANNEL_ESTABLISHED) {
    connection_status = kExternalDeviceConnected;
  } else if (life_cycle_) {
    connection_status = kExternalDeviceConnecting;
  }
  dictionary->SetString(kExternalDeviceConnectionStatus, connection_status);

  // Fill the remote status dictionary.
  if (last_remote_status_update_) {
    std::unique_ptr<base::DictionaryValue> status_dictionary(
        new base::DictionaryValue());
    status_dictionary->SetInteger("userPresent",
                                  last_remote_status_update_->user_presence);
    status_dictionary->SetInteger(
        "secureScreenLock",
        last_remote_status_update_->secure_screen_lock_state);
    status_dictionary->SetInteger(
        "trustAgent", last_remote_status_update_->trust_agent_state);
    dictionary->Set(kExternalDeviceRemoteState, std::move(status_dictionary));
  }

  return dictionary;
}

std::unique_ptr<base::DictionaryValue>
ProximityAuthWebUIHandler::IneligibleDeviceToDictionary(
    const cryptauth::IneligibleDevice& ineligible_device) {
  std::unique_ptr<base::ListValue> ineligibility_reasons(new base::ListValue());
  for (const auto& reason : ineligible_device.reasons()) {
    ineligibility_reasons->AppendInteger(reason);
  }

  std::unique_ptr<base::DictionaryValue> device_dictionary =
      ExternalDeviceInfoToDictionary(ineligible_device.device());
  device_dictionary->Set(kIneligibleDeviceReasons,
                         std::move(ineligibility_reasons));
  return device_dictionary;
}

void ProximityAuthWebUIHandler::CleanUpRemoteDeviceLifeCycle() {
  if (selected_remote_device_) {
    PA_LOG(INFO) << "Cleaning up connection to "
                 << selected_remote_device_->name();
  }
  life_cycle_.reset();
  selected_remote_device_ = base::nullopt;
  last_remote_status_update_.reset();
  web_ui()->CallJavascriptFunctionUnsafe(
      "LocalStateInterface.onRemoteDevicesChanged", *GetRemoteDevicesList());
}

void ProximityAuthWebUIHandler::OnLifeCycleStateChanged(
    RemoteDeviceLifeCycle::State old_state,
    RemoteDeviceLifeCycle::State new_state) {
  // Do not re-attempt to find a connection after the first one fails--just
  // abort.
  if ((old_state != RemoteDeviceLifeCycle::State::STOPPED &&
       new_state == RemoteDeviceLifeCycle::State::FINDING_CONNECTION) ||
      new_state == RemoteDeviceLifeCycle::State::AUTHENTICATION_FAILED) {
    // Clean up the life cycle asynchronously, because we are currently in the
    // call stack of |life_cycle_|.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&ProximityAuthWebUIHandler::CleanUpRemoteDeviceLifeCycle,
                       weak_ptr_factory_.GetWeakPtr()));
  } else if (new_state ==
             RemoteDeviceLifeCycle::State::SECURE_CHANNEL_ESTABLISHED) {
    life_cycle_->GetMessenger()->AddObserver(this);
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "LocalStateInterface.onRemoteDevicesChanged", *GetRemoteDevicesList());
}

void ProximityAuthWebUIHandler::OnRemoteStatusUpdate(
    const RemoteStatusUpdate& status_update) {
  PA_LOG(INFO) << "Remote status update:"
               << "\n  user_presence: "
               << static_cast<int>(status_update.user_presence)
               << "\n  secure_screen_lock_state: "
               << static_cast<int>(status_update.secure_screen_lock_state)
               << "\n  trust_agent_state: "
               << static_cast<int>(status_update.trust_agent_state);

  last_remote_status_update_.reset(new RemoteStatusUpdate(status_update));
  std::unique_ptr<base::ListValue> synced_devices = GetRemoteDevicesList();
  web_ui()->CallJavascriptFunctionUnsafe(
      "LocalStateInterface.onRemoteDevicesChanged", *synced_devices);
}

}  // namespace proximity_auth
