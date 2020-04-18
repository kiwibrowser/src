// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/gcm_profile_service.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/gcm_driver/gcm_driver_constants.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(USE_GCM_FROM_PLATFORM)
#include "base/sequenced_task_runner.h"
#include "components/gcm_driver/gcm_driver_android.h"
#else
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "components/gcm_driver/account_tracker.h"
#include "components/gcm_driver/gcm_account_tracker.h"
#include "components/gcm_driver/gcm_channel_status_syncer.h"
#include "components/gcm_driver/gcm_client_factory.h"
#include "components/gcm_driver/gcm_desktop_utils.h"
#include "components/gcm_driver/gcm_driver_desktop.h"
#include "net/url_request/url_request_context_getter.h"
#endif

namespace gcm {

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
// Identity observer only has actual work to do when the user is actually signed
// in. It ensures that account tracker is taking
class GCMProfileService::IdentityObserver : public SigninManagerBase::Observer {
 public:
  IdentityObserver(SigninManagerBase* signin_manager,
                   ProfileOAuth2TokenService* token_service,
                   net::URLRequestContextGetter* request_context,
                   GCMDriver* driver);
  ~IdentityObserver() override;

  // SigninManagerBase::Observer:
  void GoogleSigninSucceeded(const std::string& account_id,
                             const std::string& username) override;
  void GoogleSignedOut(const std::string& account_id,
                       const std::string& username) override;

 private:
  void StartAccountTracker(net::URLRequestContextGetter* request_context);

  GCMDriver* driver_;
  SigninManagerBase* signin_manager_;
  ProfileOAuth2TokenService* token_service_;
  std::unique_ptr<GCMAccountTracker> gcm_account_tracker_;

  // The account ID that this service is responsible for. Empty when the service
  // is not running.
  std::string account_id_;

  base::WeakPtrFactory<GCMProfileService::IdentityObserver> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(IdentityObserver);
};

GCMProfileService::IdentityObserver::IdentityObserver(
    SigninManagerBase* signin_manager,
    ProfileOAuth2TokenService* token_service,
    net::URLRequestContextGetter* request_context,
    GCMDriver* driver)
    : driver_(driver),
      signin_manager_(signin_manager),
      token_service_(token_service),
      weak_ptr_factory_(this) {
  signin_manager_->AddObserver(this);

  GoogleSigninSucceeded(signin_manager_->GetAuthenticatedAccountId(),
                        signin_manager_->GetAuthenticatedAccountInfo().email);
  StartAccountTracker(request_context);
}

GCMProfileService::IdentityObserver::~IdentityObserver() {
  if (gcm_account_tracker_)
    gcm_account_tracker_->Shutdown();
  signin_manager_->RemoveObserver(this);
}

void GCMProfileService::IdentityObserver::GoogleSigninSucceeded(
    const std::string& account_id,
    const std::string& username) {
  // This might be called multiple times when the password changes.
  if (account_id == account_id_)
    return;
  account_id_ = account_id;

  // Still need to notify GCMDriver for UMA purpose.
  driver_->OnSignedIn();
}

void GCMProfileService::IdentityObserver::GoogleSignedOut(
    const std::string& account_id,
    const std::string& username) {
  account_id_.clear();

  // Still need to notify GCMDriver for UMA purpose.
  driver_->OnSignedOut();
}

void GCMProfileService::IdentityObserver::StartAccountTracker(
    net::URLRequestContextGetter* request_context) {
  if (gcm_account_tracker_)
    return;

  std::unique_ptr<AccountTracker> gaia_account_tracker(
      new AccountTracker(signin_manager_, token_service_, request_context));

  gcm_account_tracker_.reset(new GCMAccountTracker(
      std::move(gaia_account_tracker), token_service_, driver_));

  gcm_account_tracker_->Start();
}

#endif  // !BUILDFLAG(USE_GCM_FROM_PLATFORM)

// static
bool GCMProfileService::IsGCMEnabled(PrefService* prefs) {
#if BUILDFLAG(USE_GCM_FROM_PLATFORM)
  return true;
#else
  return prefs->GetBoolean(gcm::prefs::kGCMChannelStatus);
#endif  // BUILDFLAG(USE_GCM_FROM_PLATFORM)
}

#if BUILDFLAG(USE_GCM_FROM_PLATFORM)
GCMProfileService::GCMProfileService(
    base::FilePath path,
    scoped_refptr<base::SequencedTaskRunner>& blocking_task_runner) {
  driver_.reset(new GCMDriverAndroid(path.Append(gcm_driver::kGCMStoreDirname),
                                     blocking_task_runner));
}
#else
GCMProfileService::GCMProfileService(
    PrefService* prefs,
    base::FilePath path,
    net::URLRequestContextGetter* request_context,
    version_info::Channel channel,
    const std::string& product_category_for_subtypes,
    SigninManagerBase* signin_manager,
    ProfileOAuth2TokenService* token_service,
    std::unique_ptr<GCMClientFactory> gcm_client_factory,
    const scoped_refptr<base::SequencedTaskRunner>& ui_task_runner,
    const scoped_refptr<base::SequencedTaskRunner>& io_task_runner,
    scoped_refptr<base::SequencedTaskRunner>& blocking_task_runner)
    : signin_manager_(signin_manager),
      token_service_(token_service),
      request_context_(request_context) {
  driver_ = CreateGCMDriverDesktop(
      std::move(gcm_client_factory), prefs,
      path.Append(gcm_driver::kGCMStoreDirname), request_context_, channel,
      product_category_for_subtypes, ui_task_runner, io_task_runner,
      blocking_task_runner);

  identity_observer_.reset(new IdentityObserver(
      signin_manager_, token_service_, request_context_, driver_.get()));
}
#endif  // BUILDFLAG(USE_GCM_FROM_PLATFORM)

GCMProfileService::GCMProfileService() {}

GCMProfileService::~GCMProfileService() {}

void GCMProfileService::Shutdown() {
#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  identity_observer_.reset();
#endif  // !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  if (driver_) {
    driver_->Shutdown();
    driver_.reset();
  }
}

void GCMProfileService::SetDriverForTesting(std::unique_ptr<GCMDriver> driver) {
  driver_ = std::move(driver);

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  if (identity_observer_) {
    identity_observer_ = std::make_unique<IdentityObserver>(
        signin_manager_, token_service_, request_context_, driver.get());
  }
#endif  // !BUILDFLAG(USE_GCM_FROM_PLATFORM)
}

}  // namespace gcm
