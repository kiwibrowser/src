// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/cryptauth_gcm_manager_impl.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "components/cryptauth/pref_names.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/prefs/pref_service.h"

namespace cryptauth {

namespace {

// The GCM app id identifies the client.
const char kCryptAuthGCMAppId[] = "com.google.chrome.cryptauth";

// The GCM sender id identifies the CryptAuth server.
const char kCryptAuthGCMSenderId[] = "381449029288";

// The 'registrationTickleType' key-value pair is present in GCM push
// messages. The values correspond to a server-side enum.
const char kRegistrationTickleTypeKey[] = "registrationTickleType";
const char kRegistrationTickleTypeForceEnrollment[] = "1";
const char kRegistrationTickleTypeUpdateEnrollment[] = "2";
const char kRegistrationTickleTypeDevicesSync[] = "3";

}  // namespace

// static
CryptAuthGCMManagerImpl::Factory*
    CryptAuthGCMManagerImpl::Factory::factory_instance_ = nullptr;

// static
std::unique_ptr<CryptAuthGCMManager>
CryptAuthGCMManagerImpl::Factory::NewInstance(gcm::GCMDriver* gcm_driver,
                                              PrefService* pref_service) {
  if (!factory_instance_)
    factory_instance_ = new Factory();

  return factory_instance_->BuildInstance(gcm_driver, pref_service);
}

// static
void CryptAuthGCMManagerImpl::Factory::SetInstanceForTesting(Factory* factory) {
  factory_instance_ = factory;
}

CryptAuthGCMManagerImpl::Factory::~Factory() = default;

std::unique_ptr<CryptAuthGCMManager>
CryptAuthGCMManagerImpl::Factory::BuildInstance(gcm::GCMDriver* gcm_driver,
                                                PrefService* pref_service) {
  return base::WrapUnique(
      new CryptAuthGCMManagerImpl(gcm_driver, pref_service));
}

CryptAuthGCMManagerImpl::CryptAuthGCMManagerImpl(gcm::GCMDriver* gcm_driver,
                                                 PrefService* pref_service)
    : gcm_driver_(gcm_driver),
      pref_service_(pref_service),
      registration_in_progress_(false),
      weak_ptr_factory_(this) {
}

CryptAuthGCMManagerImpl::~CryptAuthGCMManagerImpl() {
  if (gcm_driver_->GetAppHandler(kCryptAuthGCMAppId) == this)
    gcm_driver_->RemoveAppHandler(kCryptAuthGCMAppId);
}

void CryptAuthGCMManagerImpl::StartListening() {
  if (gcm_driver_->GetAppHandler(kCryptAuthGCMAppId) == this) {
    PA_LOG(INFO) << "GCM app handler already added";
    return;
  }

  gcm_driver_->AddAppHandler(kCryptAuthGCMAppId, this);
}

void CryptAuthGCMManagerImpl::RegisterWithGCM() {
  if (registration_in_progress_) {
    PA_LOG(INFO) << "GCM Registration is already in progress";
    return;
  }

  PA_LOG(INFO) << "Beginning GCM registration...";
  registration_in_progress_ = true;

  std::vector<std::string> sender_ids(1, kCryptAuthGCMSenderId);
  gcm_driver_->Register(
      kCryptAuthGCMAppId, sender_ids,
      base::Bind(&CryptAuthGCMManagerImpl::OnRegistrationCompleted,
                 weak_ptr_factory_.GetWeakPtr()));
}

std::string CryptAuthGCMManagerImpl::GetRegistrationId() {
  return pref_service_->GetString(prefs::kCryptAuthGCMRegistrationId);
}

void CryptAuthGCMManagerImpl::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void CryptAuthGCMManagerImpl::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void CryptAuthGCMManagerImpl::ShutdownHandler() {
}

void CryptAuthGCMManagerImpl::OnStoreReset() {
  // We will automatically re-register to GCM and re-enroll the new registration
  // ID to Cryptauth during the next scheduled sync.
  pref_service_->ClearPref(prefs::kCryptAuthGCMRegistrationId);
}

void CryptAuthGCMManagerImpl::OnMessage(const std::string& app_id,
                                        const gcm::IncomingMessage& message) {
  std::vector<std::string> fields;
  for (const auto& kv : message.data) {
    fields.push_back(std::string(kv.first) + ": " + std::string(kv.second));
  }

  PA_LOG(INFO) << "GCM message received:\n"
               << "  sender_id: " << message.sender_id << "\n"
               << "  collapse_key: " << message.collapse_key << "\n"
               << "  data:\n    " << base::JoinString(fields, "\n    ");

  if (message.data.find(kRegistrationTickleTypeKey) == message.data.end()) {
    PA_LOG(WARNING) << "GCM message does not contain 'registrationTickleType'.";
  } else {
    std::string tickle_type = message.data.at(kRegistrationTickleTypeKey);
    if (tickle_type == kRegistrationTickleTypeForceEnrollment ||
        tickle_type == kRegistrationTickleTypeUpdateEnrollment) {
      // These tickle types correspond to re-enrollment messages.
      for (auto& observer : observers_)
        observer.OnReenrollMessage();
    } else if (tickle_type == kRegistrationTickleTypeDevicesSync) {
      for (auto& observer : observers_)
        observer.OnResyncMessage();
    } else {
      PA_LOG(WARNING) << "Unknown tickle type in GCM message.";
    }
  }
}

void CryptAuthGCMManagerImpl::OnMessagesDeleted(const std::string& app_id) {
}

void CryptAuthGCMManagerImpl::OnSendError(
    const std::string& app_id,
    const gcm::GCMClient::SendErrorDetails& details) {
  NOTREACHED();
}

void CryptAuthGCMManagerImpl::OnSendAcknowledged(
    const std::string& app_id,
    const std::string& message_id) {
  NOTREACHED();
}

void CryptAuthGCMManagerImpl::OnRegistrationCompleted(
    const std::string& registration_id,
    gcm::GCMClient::Result result) {
  registration_in_progress_ = false;
  if (result != gcm::GCMClient::SUCCESS) {
    PA_LOG(WARNING) << "GCM registration failed with result="
                    << static_cast<int>(result);
    for (auto& observer : observers_)
      observer.OnGCMRegistrationResult(false);
    return;
  }

  PA_LOG(INFO) << "GCM registration success, registration_id="
               << registration_id;
  pref_service_->SetString(prefs::kCryptAuthGCMRegistrationId, registration_id);
  for (auto& observer : observers_)
    observer.OnGCMRegistrationResult(true);
}

}  // namespace cryptauth
