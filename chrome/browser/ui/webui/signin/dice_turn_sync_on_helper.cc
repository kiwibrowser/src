// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/dice_turn_sync_on_helper.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/policy/cloud/user_policy_signin_service.h"
#include "chrome/browser/policy/cloud/user_policy_signin_service_factory.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/signin/signin_util.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/webui/signin/dice_turn_sync_on_helper_delegate_impl.h"
#include "chrome/browser/ui/webui/signin/signin_utils_desktop.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/account_info.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "components/sync/base/sync_prefs.h"
#include "net/url_request/url_request_context_getter.h"

namespace {

AccountInfo GetAccountInfo(Profile* profile, const std::string& account_id) {
  return AccountTrackerServiceFactory::GetForProfile(profile)->GetAccountInfo(
      account_id);
}

}  // namespace

DiceTurnSyncOnHelper::DiceTurnSyncOnHelper(
    Profile* profile,
    signin_metrics::AccessPoint signin_access_point,
    signin_metrics::PromoAction signin_promo_action,
    signin_metrics::Reason signin_reason,
    const std::string& account_id,
    SigninAbortedMode signin_aborted_mode,
    std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)),
      profile_(profile),
      signin_manager_(SigninManagerFactory::GetForProfile(profile)),
      token_service_(ProfileOAuth2TokenServiceFactory::GetForProfile(profile)),
      signin_access_point_(signin_access_point),
      signin_promo_action_(signin_promo_action),
      signin_reason_(signin_reason),
      signin_aborted_mode_(signin_aborted_mode),
      account_info_(GetAccountInfo(profile, account_id)),
      weak_pointer_factory_(this) {
  DCHECK(delegate_);
  DCHECK(signin::IsDicePrepareMigrationEnabled());
  DCHECK(profile_);
  // Should not start syncing if the profile is already authenticated
  DCHECK(!signin_manager_->IsAuthenticated());

  // Force sign-in uses the modal sign-in flow.
  DCHECK(!signin_util::IsForceSigninEnabled());

  if (account_info_.gaia.empty() || account_info_.email.empty()) {
    LOG(ERROR) << "Cannot turn Sync On for invalid account.";
    base::SequencedTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
    return;
  }

  DCHECK(!account_info_.gaia.empty());
  DCHECK(!account_info_.email.empty());

  if (HasCanOfferSigninError()) {
    // Do not self-destruct synchronously in the constructor.
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&DiceTurnSyncOnHelper::AbortAndDelete,
                                  base::Unretained(this)));
    return;
  }

  if (!IsCrossAccountError(profile_, account_info_.email, account_info_.gaia)) {
    TurnSyncOnWithProfileMode(ProfileMode::CURRENT_PROFILE);
    return;
  }

  // Handles cross account sign in error. If |account_info_| does not match the
  // last authenticated account of the current profile, then Chrome will show a
  // confirmation dialog before starting sync.
  // TODO(skym): Warn for high risk upgrade scenario (https://crbug.com/572754).
  std::string last_email =
      profile_->GetPrefs()->GetString(prefs::kGoogleServicesLastUsername);
  delegate_->ShowMergeSyncDataConfirmation(
      last_email, account_info_.email,
      base::BindOnce(&DiceTurnSyncOnHelper::OnMergeAccountConfirmation,
                     weak_pointer_factory_.GetWeakPtr()));
}

DiceTurnSyncOnHelper::DiceTurnSyncOnHelper(
    Profile* profile,
    Browser* browser,
    signin_metrics::AccessPoint signin_access_point,
    signin_metrics::PromoAction signin_promo_action,
    signin_metrics::Reason signin_reason,
    const std::string& account_id,
    SigninAbortedMode signin_aborted_mode)
    : DiceTurnSyncOnHelper(
          profile,
          signin_access_point,
          signin_promo_action,
          signin_reason,
          account_id,
          signin_aborted_mode,
          std::make_unique<DiceTurnSyncOnHelperDelegateImpl>(browser)) {}

DiceTurnSyncOnHelper::~DiceTurnSyncOnHelper() {
}

bool DiceTurnSyncOnHelper::HasCanOfferSigninError() {
  std::string error_msg;
  bool can_offer =
      CanOfferSignin(profile_, CAN_OFFER_SIGNIN_FOR_ALL_ACCOUNTS,
                     account_info_.gaia, account_info_.email, &error_msg);
  if (can_offer)
    return false;

  // Display the error message
  delegate_->ShowLoginError(account_info_.email, error_msg);
  return true;
}

void DiceTurnSyncOnHelper::OnMergeAccountConfirmation(SigninChoice choice) {
  switch (choice) {
    case SIGNIN_CHOICE_NEW_PROFILE:
      base::RecordAction(
          base::UserMetricsAction("Signin_ImportDataPrompt_DontImport"));
      TurnSyncOnWithProfileMode(ProfileMode::NEW_PROFILE);
      break;
    case SIGNIN_CHOICE_CONTINUE:
      base::RecordAction(
          base::UserMetricsAction("Signin_ImportDataPrompt_ImportData"));
      TurnSyncOnWithProfileMode(ProfileMode::CURRENT_PROFILE);
      break;
    case SIGNIN_CHOICE_CANCEL:
      base::RecordAction(
          base::UserMetricsAction("Signin_ImportDataPrompt_Cancel"));
      AbortAndDelete();
      break;
    case SIGNIN_CHOICE_SIZE:
      NOTREACHED();
      AbortAndDelete();
      break;
  }
}

void DiceTurnSyncOnHelper::OnEnterpriseAccountConfirmation(
    SigninChoice choice) {
  UMA_HISTOGRAM_ENUMERATION("Enterprise.UserSigninChoice", choice,
                            DiceTurnSyncOnHelper::SIGNIN_CHOICE_SIZE);
  switch (choice) {
    case SIGNIN_CHOICE_CANCEL:
      base::RecordAction(
          base::UserMetricsAction("Signin_EnterpriseAccountPrompt_Cancel"));
      AbortAndDelete();
      break;
    case SIGNIN_CHOICE_CONTINUE:
      base::RecordAction(
          base::UserMetricsAction("Signin_EnterpriseAccountPrompt_ImportData"));
      LoadPolicyWithCachedCredentials();
      break;
    case SIGNIN_CHOICE_NEW_PROFILE:
      base::RecordAction(base::UserMetricsAction(
          "Signin_EnterpriseAccountPrompt_DontImportData"));
      CreateNewSignedInProfile();
      break;
    case SIGNIN_CHOICE_SIZE:
      NOTREACHED();
      AbortAndDelete();
      break;
  }
}

void DiceTurnSyncOnHelper::TurnSyncOnWithProfileMode(ProfileMode profile_mode) {
  // Make sure the syncing is requested, otherwise the SigninManager
  // will not be able to complete successfully.
  syncer::SyncPrefs sync_prefs(profile_->GetPrefs());
  sync_prefs.SetSyncRequested(true);

  switch (profile_mode) {
    case ProfileMode::CURRENT_PROFILE: {
      // If this is a new signin (no account authenticated yet) try loading
      // policy for this user now, before any signed in services are
      // initialized.
      policy::UserPolicySigninService* policy_service =
          policy::UserPolicySigninServiceFactory::GetForProfile(profile_);
      policy_service->RegisterForPolicyWithAccountId(
          account_info_.email, account_info_.account_id,
          base::Bind(&DiceTurnSyncOnHelper::OnRegisteredForPolicy,
                     weak_pointer_factory_.GetWeakPtr()));
      break;
    }
    case ProfileMode::NEW_PROFILE:
      // If this is a new signin (no account authenticated yet) in a new
      // profile, then just create the new signed-in profile and skip loading
      // the policy as there is no need to ask the user again if they should be
      // signed in to a new profile. Note that in this case the policy will be
      // applied after the new profile is signed in.
      CreateNewSignedInProfile();
      break;
  }
}

void DiceTurnSyncOnHelper::OnRegisteredForPolicy(const std::string& dm_token,
                                                 const std::string& client_id) {
  // If there's no token for the user (policy registration did not succeed) just
  // finish signing in.
  if (dm_token.empty()) {
    DVLOG(1) << "Policy registration failed";
    SigninAndShowSyncConfirmationUI();
    return;
  }

  DVLOG(1) << "Policy registration succeeded: dm_token=" << dm_token;

  DCHECK(dm_token_.empty());
  DCHECK(client_id_.empty());
  dm_token_ = dm_token;
  client_id_ = client_id;

  // Allow user to create a new profile before continuing with sign-in.
  delegate_->ShowEnterpriseAccountConfirmation(
      account_info_.email,
      base::BindOnce(&DiceTurnSyncOnHelper::OnEnterpriseAccountConfirmation,
                     weak_pointer_factory_.GetWeakPtr()));
}

void DiceTurnSyncOnHelper::LoadPolicyWithCachedCredentials() {
  DCHECK(!dm_token_.empty());
  DCHECK(!client_id_.empty());
  policy::UserPolicySigninService* policy_service =
      policy::UserPolicySigninServiceFactory::GetForProfile(profile_);
  policy_service->FetchPolicyForSignedInUser(
      account_info_.GetAccountId(), dm_token_, client_id_,
      profile_->GetRequestContext(),
      base::Bind(&DiceTurnSyncOnHelper::OnPolicyFetchComplete,
                 weak_pointer_factory_.GetWeakPtr()));
}

void DiceTurnSyncOnHelper::OnPolicyFetchComplete(bool success) {
  // For now, we allow signin to complete even if the policy fetch fails. If
  // we ever want to change this behavior, we could call
  // SigninManager::SignOut() here instead.
  DLOG_IF(ERROR, !success) << "Error fetching policy for user";
  DVLOG_IF(1, success) << "Policy fetch successful - completing signin";
  SigninAndShowSyncConfirmationUI();
}

void DiceTurnSyncOnHelper::CreateNewSignedInProfile() {
  // Create a new profile and have it call back when done so we can start the
  // signin flow.
  size_t icon_index = g_browser_process->profile_manager()
                          ->GetProfileAttributesStorage()
                          .ChooseAvatarIconIndexForNewProfile();
  ProfileManager::CreateMultiProfileAsync(
      base::UTF8ToUTF16(account_info_.email),
      profiles::GetDefaultAvatarIconUrl(icon_index),
      base::BindRepeating(&DiceTurnSyncOnHelper::CompleteInitForNewProfile,
                          weak_pointer_factory_.GetWeakPtr()),
      std::string());
}

void DiceTurnSyncOnHelper::CompleteInitForNewProfile(
    Profile* new_profile,
    Profile::CreateStatus status) {
  DCHECK_NE(profile_, new_profile);

  // TODO(atwilson): On error, unregister the client to release the DMToken
  // and surface a better error for the user.
  switch (status) {
    case Profile::CREATE_STATUS_LOCAL_FAIL:
      NOTREACHED() << "Error creating new profile";
      AbortAndDelete();
      break;
    case Profile::CREATE_STATUS_CREATED:
      // Ignore this, wait for profile to be initialized.
      break;
    case Profile::CREATE_STATUS_INITIALIZED:
      // The user needs to sign in to the new profile in order to enable sync.
      delegate_->ShowSigninPageInNewProfile(new_profile, account_info_.email);
      AbortAndDelete();
      break;
    case Profile::CREATE_STATUS_REMOTE_FAIL:
    case Profile::CREATE_STATUS_CANCELED:
    case Profile::MAX_CREATE_STATUS: {
      NOTREACHED() << "Invalid profile creation status";
      AbortAndDelete();
      break;
    }
  }
}

browser_sync::ProfileSyncService*
DiceTurnSyncOnHelper::GetProfileSyncService() {
  return profile_->IsSyncAllowed()
             ? ProfileSyncServiceFactory::GetForProfile(profile_)
             : nullptr;
}

void DiceTurnSyncOnHelper::SigninAndShowSyncConfirmationUI() {
  // Signin.
  signin_manager_->OnExternalSigninCompleted(account_info_.email);
  signin_metrics::LogSigninAccessPointCompleted(signin_access_point_,
                                                signin_promo_action_);
  signin_metrics::LogSigninReason(signin_reason_);
  base::RecordAction(base::UserMetricsAction("Signin_Signin_Succeed"));

  browser_sync::ProfileSyncService* sync_service = GetProfileSyncService();
  if (sync_service) {
    // Take a SyncSetupInProgressHandle, so that the UI code can use
    // IsFirstSyncSetupInProgress() as a way to know if there is a signin in
    // progress.
    // TODO(https://crbug.com/811211): Remove this handle.
    sync_blocker_ = sync_service->GetSetupInProgressHandle();
    bool is_enterprise_user =
        !policy::BrowserPolicyConnector::IsNonEnterpriseUser(
            account_info_.email);
    if (is_enterprise_user &&
        SyncStartupTracker::GetSyncServiceState(profile_) ==
            SyncStartupTracker::SYNC_STARTUP_PENDING) {
      // For enterprise users it is important to wait until sync is initialized
      // so that the confirmation UI can be
      // aware of startup errors. This is needed to make sure that the sync
      // confirmation dialog is shown only after the sync service had a chance
      // to check whether sync was disabled by admin.
      // See http://crbug.com/812546
      sync_startup_tracker_.reset(new SyncStartupTracker(profile_, this));
      return;
    }
  }

  ShowSyncConfirmationUI();
}

void DiceTurnSyncOnHelper::SyncStartupCompleted() {
  DCHECK(sync_startup_tracker_);
  sync_startup_tracker_.reset();
  ShowSyncConfirmationUI();
}

void DiceTurnSyncOnHelper::SyncStartupFailed() {
  DCHECK(sync_startup_tracker_);
  sync_startup_tracker_.reset();
  ShowSyncConfirmationUI();
}

void DiceTurnSyncOnHelper::ShowSyncConfirmationUI() {
  delegate_->ShowSyncConfirmation(
      base::BindOnce(&DiceTurnSyncOnHelper::FinishSyncSetupAndDelete,
                     weak_pointer_factory_.GetWeakPtr()));
}

void DiceTurnSyncOnHelper::FinishSyncSetupAndDelete(
    LoginUIService::SyncConfirmationUIClosedResult result) {
  switch (result) {
    case LoginUIService::CONFIGURE_SYNC_FIRST:
      delegate_->ShowSyncSettings();
      break;
    case LoginUIService::SYNC_WITH_DEFAULT_SETTINGS: {
      browser_sync::ProfileSyncService* sync_service = GetProfileSyncService();
      if (sync_service)
        sync_service->SetFirstSetupComplete();
      break;
    }
    case LoginUIService::ABORT_SIGNIN:
      signin_manager_->SignOutAndKeepAllAccounts(
          signin_metrics::ABORT_SIGNIN,
          signin_metrics::SignoutDelete::IGNORE_METRIC);
      AbortAndDelete();
      return;
  }
  delete this;
}

void DiceTurnSyncOnHelper::AbortAndDelete() {
  if (signin_aborted_mode_ == SigninAbortedMode::REMOVE_ACCOUNT) {
    // Revoke the token, and the AccountReconcilor and/or the Gaia server will
    // take care of invalidating the cookies.
    token_service_->RevokeCredentials(account_info_.account_id);
  }
  delete this;
}
