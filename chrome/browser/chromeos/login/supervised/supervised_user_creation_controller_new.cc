// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/supervised/supervised_user_creation_controller_new.h"

#include "base/base64.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/sys_info.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/values.h"
#include "chrome/browser/chromeos/login/supervised/supervised_user_authentication.h"
#include "chrome/browser/chromeos/login/supervised/supervised_user_constants.h"
#include "chrome/browser/chromeos/login/users/chrome_user_manager.h"
#include "chrome/browser/chromeos/login/users/supervised_user_manager.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "chromeos/login/auth/key.h"
#include "chromeos/login/auth/user_context.h"
#include "components/account_id/account_id.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "crypto/random.h"
#include "google_apis/gaia/google_service_auth_error.h"

namespace chromeos {

SupervisedUserCreationControllerNew::SupervisedUserCreationControllerNew(
    SupervisedUserCreationControllerNew::StatusConsumer* consumer,
    const AccountId& manager_id)
    : SupervisedUserCreationController(consumer),
      stage_(STAGE_INITIAL),
      weak_factory_(this) {
  creation_context_.reset(
      new SupervisedUserCreationControllerNew::UserCreationContext());
  creation_context_->manager_id = manager_id;
}

SupervisedUserCreationControllerNew::~SupervisedUserCreationControllerNew() {}

SupervisedUserCreationControllerNew::UserCreationContext::
    UserCreationContext() {}

SupervisedUserCreationControllerNew::UserCreationContext::
    ~UserCreationContext() {}

void SupervisedUserCreationControllerNew::SetManagerProfile(
    Profile* manager_profile) {
  creation_context_->manager_profile = manager_profile;
}

Profile* SupervisedUserCreationControllerNew::GetManagerProfile() {
  return creation_context_->manager_profile;
}

void SupervisedUserCreationControllerNew::OnAuthenticationFailure(
    ExtendedAuthenticator::AuthState error) {
  timeout_timer_.Stop();
  ErrorCode code = NO_ERROR;
  switch (error) {
    case ExtendedAuthenticator::NO_MOUNT:
      code = CRYPTOHOME_NO_MOUNT;
      break;
    case ExtendedAuthenticator::FAILED_MOUNT:
      code = CRYPTOHOME_FAILED_MOUNT;
      break;
    case ExtendedAuthenticator::FAILED_TPM:
      code = CRYPTOHOME_FAILED_TPM;
      break;
    default:
      NOTREACHED();
  }
  stage_ = STAGE_ERROR;
  if (consumer_)
    consumer_->OnCreationError(code);
}

void SupervisedUserCreationControllerNew::OnSupervisedUserFilesStored(
    bool success) {
  DCHECK(creation_context_);
  DCHECK_EQ(DASHBOARD_CREATED, stage_);

  if (!success) {
    stage_ = STAGE_ERROR;
    if (consumer_)
      consumer_->OnCreationError(TOKEN_WRITE_FAILED);
    return;
  }
  // Assume that new token is valid. It will be automatically invalidated if
  // sync service fails to use it.
  user_manager::UserManager::Get()->SaveUserOAuthStatus(
      AccountId::FromUserEmail(creation_context_->local_user_id),
      user_manager::User::OAUTH2_TOKEN_STATUS_VALID);

  stage_ = TOKEN_WRITTEN;

  timeout_timer_.Stop();
  ChromeUserManager::Get()
      ->GetSupervisedUserManager()
      ->CommitCreationTransaction();
  base::RecordAction(
      base::UserMetricsAction("ManagedMode_LocallyManagedUserCreated"));

  stage_ = TRANSACTION_COMMITTED;

  if (consumer_)
    consumer_->OnCreationSuccess();
}

void SupervisedUserCreationControllerNew::CreationTimedOut() {
  LOG(ERROR) << "Supervised user creation timed out. stage = " << stage_;
  if (consumer_)
    consumer_->OnCreationTimeout();
}

void SupervisedUserCreationControllerNew::FinishCreation() {
  chrome::AttemptUserExit();
}

void SupervisedUserCreationControllerNew::CancelCreation() {
  chrome::AttemptUserExit();
}

std::string SupervisedUserCreationControllerNew::GetSupervisedUserId() {
  DCHECK(creation_context_);
  return creation_context_->local_user_id;
}

}  // namespace chromeos
