// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/cryptauth/chrome_cryptauth_service.h"

#include "base/guid.h"
#include "base/linux_util.h"
#include "base/memory/ptr_util.h"
#include "base/sys_info.h"
#include "base/time/default_clock.h"
#include "base/version.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/chromeos/cryptauth/cryptauth_device_id_provider_impl.h"
#include "chrome/browser/chromeos/cryptauth/gcm_device_info_provider_impl.h"
#include "chrome/browser/gcm/gcm_profile_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/common/pref_names.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "components/cryptauth/cryptauth_client.h"
#include "components/cryptauth/cryptauth_client_impl.h"
#include "components/cryptauth/cryptauth_device_manager_impl.h"
#include "components/cryptauth/cryptauth_enroller.h"
#include "components/cryptauth/cryptauth_enroller_impl.h"
#include "components/cryptauth/cryptauth_enrollment_manager_impl.h"
#include "components/cryptauth/cryptauth_enrollment_utils.h"
#include "components/cryptauth/cryptauth_gcm_manager_impl.h"
#include "components/cryptauth/device_classifier_util.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/secure_message_delegate_impl.h"
#include "components/gcm_driver/gcm_profile_service.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "components/version_info/version_info.h"
#include "ui/gfx/geometry/rect.h"

namespace chromeos {

namespace {

std::unique_ptr<cryptauth::CryptAuthClientFactory>
CreateCryptAuthClientFactoryImpl(Profile* profile) {
  return std::make_unique<cryptauth::CryptAuthClientFactoryImpl>(
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile),
      SigninManagerFactory::GetForProfile(profile)->GetAuthenticatedAccountId(),
      profile->GetRequestContext(),
      cryptauth::device_classifier_util::GetDeviceClassifier());
}

class CryptAuthEnrollerFactoryImpl
    : public cryptauth::CryptAuthEnrollerFactory {
 public:
  explicit CryptAuthEnrollerFactoryImpl(
      cryptauth::CryptAuthClientFactory* client_factory)
      : client_factory_(client_factory) {}

  std::unique_ptr<cryptauth::CryptAuthEnroller> CreateInstance() override {
    return std::make_unique<cryptauth::CryptAuthEnrollerImpl>(
        client_factory_,
        cryptauth::SecureMessageDelegateImpl::Factory::NewInstance());
  }

 private:
  cryptauth::CryptAuthClientFactory* client_factory_;
};

}  // namespace

// static
std::unique_ptr<ChromeCryptAuthService> ChromeCryptAuthService::Create(
    Profile* profile) {
  std::unique_ptr<cryptauth::CryptAuthClientFactory> client_factory =
      CreateCryptAuthClientFactoryImpl(profile);

  std::unique_ptr<cryptauth::CryptAuthGCMManager> gcm_manager =
      cryptauth::CryptAuthGCMManagerImpl::Factory::NewInstance(
          gcm::GCMProfileServiceFactory::GetForProfile(profile)->driver(),
          profile->GetPrefs());

  std::unique_ptr<cryptauth::CryptAuthDeviceManager> device_manager =
      cryptauth::CryptAuthDeviceManagerImpl::Factory::NewInstance(
          base::DefaultClock::GetInstance(), client_factory.get(),
          gcm_manager.get(), profile->GetPrefs());

  std::unique_ptr<cryptauth::CryptAuthEnrollmentManager> enrollment_manager =
      cryptauth::CryptAuthEnrollmentManagerImpl::Factory::NewInstance(
          base::DefaultClock::GetInstance(),
          std::make_unique<CryptAuthEnrollerFactoryImpl>(client_factory.get()),
          cryptauth::SecureMessageDelegateImpl::Factory::NewInstance(),
          GcmDeviceInfoProviderImpl::GetInstance()->GetGcmDeviceInfo(),
          gcm_manager.get(), profile->GetPrefs());

  // Note: ChromeCryptAuthServiceFactory DependsOn(OAuth2TokenServiceFactory),
  // so |token_service| is guaranteed to outlast this service.
  ProfileOAuth2TokenService* token_service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile);

  // Note: ChromeCryptAuthServiceFactory DependsOn(OAuth2TokenServiceFactory),
  // so |token_service| is guaranteed to outlast this service.
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(profile);

  return base::WrapUnique(new ChromeCryptAuthService(
      std::move(client_factory), std::move(gcm_manager),
      std::move(device_manager), std::move(enrollment_manager), profile,
      token_service, signin_manager));
}

ChromeCryptAuthService::ChromeCryptAuthService(
    std::unique_ptr<cryptauth::CryptAuthClientFactory> client_factory,
    std::unique_ptr<cryptauth::CryptAuthGCMManager> gcm_manager,
    std::unique_ptr<cryptauth::CryptAuthDeviceManager> device_manager,
    std::unique_ptr<cryptauth::CryptAuthEnrollmentManager> enrollment_manager,
    Profile* profile,
    OAuth2TokenService* token_service,
    SigninManagerBase* signin_manager)
    : KeyedService(),
      cryptauth::CryptAuthService(),
      client_factory_(std::move(client_factory)),
      gcm_manager_(std::move(gcm_manager)),
      enrollment_manager_(std::move(enrollment_manager)),
      device_manager_(std::move(device_manager)),
      profile_(profile),
      token_service_(token_service),
      signin_manager_(signin_manager),
      weak_ptr_factory_(this) {
  gcm_manager_->StartListening();

  registrar_.Init(profile_->GetPrefs());
  registrar_.Add(prefs::kEasyUnlockAllowed,
                 base::Bind(&ChromeCryptAuthService::OnPrefsChanged,
                            weak_ptr_factory_.GetWeakPtr()));
  registrar_.Add(prefs::kInstantTetheringAllowed,
                 base::Bind(&ChromeCryptAuthService::OnPrefsChanged,
                            weak_ptr_factory_.GetWeakPtr()));

  if (!signin_manager_->IsAuthenticated()) {
    PA_LOG(INFO) << "Profile is not authenticated yet; "
                 << "waiting before starting CryptAuth managers.";
    signin_manager_->AddObserver(this);
    return;
  }

  std::string account_id = signin_manager_->GetAuthenticatedAccountId();
  if (!token_service_->RefreshTokenIsAvailable(account_id)) {
    PA_LOG(INFO) << "Refresh token not yet available; "
                 << "waiting before starting CryptAuth managers.";
    token_service_->AddObserver(this);
    return;
  }

  // Profile is authenticated and there is a refresh token available for the
  // authenticated account id.
  PerformEnrollmentAndDeviceSyncIfPossible();
}

ChromeCryptAuthService::~ChromeCryptAuthService() {}

void ChromeCryptAuthService::Shutdown() {
  signin_manager_->RemoveObserver(this);
  token_service_->RemoveObserver(this);
  enrollment_manager_.reset();
  device_manager_.reset();
  gcm_manager_.reset();
}

cryptauth::CryptAuthDeviceManager*
ChromeCryptAuthService::GetCryptAuthDeviceManager() {
  return device_manager_.get();
}

cryptauth::CryptAuthEnrollmentManager*
ChromeCryptAuthService::GetCryptAuthEnrollmentManager() {
  return enrollment_manager_.get();
}

cryptauth::DeviceClassifier ChromeCryptAuthService::GetDeviceClassifier() {
  return cryptauth::device_classifier_util::GetDeviceClassifier();
}

std::string ChromeCryptAuthService::GetAccountId() {
  return signin_manager_->GetAuthenticatedAccountId();
}

std::unique_ptr<cryptauth::CryptAuthClientFactory>
ChromeCryptAuthService::CreateCryptAuthClientFactory() {
  return CreateCryptAuthClientFactoryImpl(profile_);
}

void ChromeCryptAuthService::OnEnrollmentFinished(bool success) {
  if (success)
    device_manager_->Start();
  else
    PA_LOG(ERROR) << "CryptAuth enrollment failed. Device manager was not "
                  << " started.";

  enrollment_manager_->RemoveObserver(this);
}

void ChromeCryptAuthService::GoogleSigninSucceeded(
    const std::string& account_id,
    const std::string& username) {
  signin_manager_->RemoveObserver(this);
  if (!token_service_->RefreshTokenIsAvailable(account_id)) {
    PA_LOG(INFO) << "Refresh token not yet available; "
                 << "waiting before starting CryptAuth managers.";
    token_service_->AddObserver(this);
    return;
  }

  PerformEnrollmentAndDeviceSyncIfPossible();
}

void ChromeCryptAuthService::OnRefreshTokenAvailable(
    const std::string& account_id) {
  if (account_id == GetAccountId()) {
    token_service_->RemoveObserver(this);
    PerformEnrollmentAndDeviceSyncIfPossible();
  }
}

void ChromeCryptAuthService::PerformEnrollmentAndDeviceSyncIfPossible() {
  DCHECK(signin_manager_->IsAuthenticated());
  DCHECK(token_service_->RefreshTokenIsAvailable(GetAccountId()));

  if (!IsEnrollmentAllowedByPolicy()) {
    PA_LOG(INFO) << "CryptAuth enrollment is disabled by enterprise policy.";
    return;
  }

  if (enrollment_manager_->IsEnrollmentValid()) {
    device_manager_->Start();
  } else {
    // If enrollment is not valid, wait for the new enrollment attempt to finish
    // before starting CryptAuthDeviceManager. See OnEnrollmentFinished(),
    enrollment_manager_->AddObserver(this);
  }

  // Even if enrollment was valid, CryptAuthEnrollmentManager must be started in
  // order to schedule the next enrollment attempt.
  enrollment_manager_->Start();
}

bool ChromeCryptAuthService::IsEnrollmentAllowedByPolicy() {
  // We allow CryptAuth enrollments if at least one of the features which
  // depends on CryptAuth is enabled by enterprise policy.
  PrefService* pref_service = profile_->GetPrefs();
  return pref_service->GetBoolean(prefs::kEasyUnlockAllowed) |
         pref_service->GetBoolean(prefs::kInstantTetheringAllowed);
}

void ChromeCryptAuthService::OnPrefsChanged() {
  // Note: We only start the CryptAuth services if a feature was toggled on. In
  // the inverse case, we simply leave the services running until the user logs
  // off.
  PerformEnrollmentAndDeviceSyncIfPossible();
}

}  // namespace chromeos
