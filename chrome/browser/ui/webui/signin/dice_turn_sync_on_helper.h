// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SIGNIN_DICE_TURN_SYNC_ON_HELPER_H_
#define CHROME_BROWSER_UI_WEBUI_SIGNIN_DICE_TURN_SYNC_ON_HELPER_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/sync_startup_tracker.h"
#include "chrome/browser/ui/webui/signin/login_ui_service.h"
#include "components/signin/core/browser/account_info.h"
#include "components/signin/core/browser/signin_metrics.h"

class Browser;
class ProfileOAuth2TokenService;
class SigninManager;

namespace browser_sync {
class ProfileSyncService;
}

namespace syncer {
class SyncSetupInProgressHandle;
}

// Handles details of signing the user in with SigninManager and turning on
// sync for an account that is already present in the token service.
class DiceTurnSyncOnHelper : public SyncStartupTracker::Observer {
 public:
  // Behavior when the signin is aborted (by an error or cancelled by the user).
  enum class SigninAbortedMode {
    // The token is revoked and the account is signed out of the web.
    REMOVE_ACCOUNT,
    // The account is kept.
    KEEP_ACCOUNT
  };

  // User choice when signing in.
  // Used for UMA histograms, Hence, constants should never be deleted or
  // reordered, and  new constants should only be appended at the end.
  // Keep this in sync with SigninChoice in histograms.xml.
  enum SigninChoice {
    SIGNIN_CHOICE_CANCEL = 0,       // Signin is cancelled.
    SIGNIN_CHOICE_CONTINUE = 1,     // Signin continues in the current profile.
    SIGNIN_CHOICE_NEW_PROFILE = 2,  // Signin continues in a new profile.
    // SIGNIN_CHOICE_SIZE should always be last.
    SIGNIN_CHOICE_SIZE,
  };

  using SigninChoiceCallback = base::OnceCallback<void(SigninChoice)>;

  // Delegate implementing the UI prompts.
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Shows a login error to the user.
    virtual void ShowLoginError(const std::string& email,
                                const std::string& error_message) = 0;

    // Shows a confirmation dialog when the user was previously signed in with a
    // different account in the same profile. |callback| must be called.
    virtual void ShowMergeSyncDataConfirmation(
        const std::string& previous_email,
        const std::string& new_email,
        SigninChoiceCallback callback) = 0;

    // Shows a confirmation dialog when the user is signing in a managed
    // account. |callback| must be called.
    virtual void ShowEnterpriseAccountConfirmation(
        const std::string& email,
        SigninChoiceCallback callback) = 0;

    // Shows a sync confirmation screen offering to open the Sync settings.
    // |callback| must be called.
    virtual void ShowSyncConfirmation(
        base::OnceCallback<void(LoginUIService::SyncConfirmationUIClosedResult)>
            callback) = 0;

    // Opens the Sync settings page.
    virtual void ShowSyncSettings() = 0;

    // Opens the signin page in a new profile.
    virtual void ShowSigninPageInNewProfile(Profile* new_profile,
                                            const std::string& username) = 0;
  };

  // Create a helper that turns sync on for an account that is already present
  // in the token service.
  DiceTurnSyncOnHelper(Profile* profile,
                       signin_metrics::AccessPoint signin_access_point,
                       signin_metrics::PromoAction signin_promo_action,
                       signin_metrics::Reason signin_reason,
                       const std::string& account_id,
                       SigninAbortedMode signin_aborted_mode,
                       std::unique_ptr<Delegate> delegate);

  // Convenience constructor using the default delegate.
  DiceTurnSyncOnHelper(Profile* profile,
                       Browser* browser,
                       signin_metrics::AccessPoint signin_access_point,
                       signin_metrics::PromoAction signin_promo_action,
                       signin_metrics::Reason signin_reason,
                       const std::string& account_id,
                       SigninAbortedMode signin_aborted_mode);

  // SyncStartupTracker::Observer:
  void SyncStartupCompleted() override;
  void SyncStartupFailed() override;

 private:
  friend class base::DeleteHelper<DiceTurnSyncOnHelper>;

  enum class ProfileMode {
    // Attempts to sign the user in |profile_|. Note that if the account to be
    // signed in is a managed account, then a profile confirmation dialog is
    // shown and the user has the possibility to create a new profile before
    // signing in.
    CURRENT_PROFILE,

    // Creates a new profile and signs the user in this new profile.
    NEW_PROFILE
  };

  // DiceTurnSyncOnHelper deletes itself.
  ~DiceTurnSyncOnHelper() override;

  // Handles can offer sign-in errors.  It returns true if there is an error,
  // and false otherwise.
  bool HasCanOfferSigninError();

  // Used as callback for ShowMergeSyncDataConfirmation().
  void OnMergeAccountConfirmation(SigninChoice choice);

  // Used as callback for ShowEnterpriseAccountConfirmation().
  void OnEnterpriseAccountConfirmation(SigninChoice choice);

  // Turns sync on with the current profile or a new profile.
  void TurnSyncOnWithProfileMode(ProfileMode profile_mode);

  // Callback invoked once policy registration is complete. If registration
  // fails, |dm_token| and |client_id| will be empty.
  void OnRegisteredForPolicy(const std::string& dm_token,
                             const std::string& client_id);

  // Helper function that loads policy with the cached |dm_token_| and
  // |client_id|, then completes the signin process.
  void LoadPolicyWithCachedCredentials();

  // Callback invoked when a policy fetch request has completed. |success| is
  // true if policy was successfully fetched.
  void OnPolicyFetchComplete(bool success);

  // Called to create a new profile, which is then signed in with the
  // in-progress auth credentials currently stored in this object.
  void CreateNewSignedInProfile();

  // Callback invoked once a profile is created, so we can complete the
  // credentials transfer, load policy, and open the first window.
  void CompleteInitForNewProfile(Profile* new_profile,
                                 Profile::CreateStatus status);

  // Returns the ProfileSyncService, or nullptr if sync is not allowed.
  browser_sync::ProfileSyncService* GetProfileSyncService();

  // Completes the signin in SigninManager and displays the Sync confirmation
  // UI.
  void SigninAndShowSyncConfirmationUI();

  // Displays the Sync confirmation UI.
  // Note: If sync fails to start (e.g. sync is disabled by admin), the sync
  // confirmation dialog will be updated accordingly.
  void ShowSyncConfirmationUI();

  // Handles the user input from the sync confirmation UI and deletes this
  // object.
  void FinishSyncSetupAndDelete(
      LoginUIService::SyncConfirmationUIClosedResult result);

  // Aborts the flow and deletes this object.
  void AbortAndDelete();

  std::unique_ptr<Delegate> delegate_;
  Profile* profile_;
  SigninManager* signin_manager_;
  ProfileOAuth2TokenService* token_service_;
  const signin_metrics::AccessPoint signin_access_point_;
  const signin_metrics::PromoAction signin_promo_action_;
  const signin_metrics::Reason signin_reason_;

  // Whether the refresh token should be deleted if the Sync flow is aborted.
  const SigninAbortedMode signin_aborted_mode_;

  // Account information.
  const AccountInfo account_info_;

  // Prevents Sync from running until configuration is complete.
  std::unique_ptr<syncer::SyncSetupInProgressHandle> sync_blocker_;

  // Policy credentials we keep while determining whether to create
  // a new profile for an enterprise user or not.
  std::string dm_token_;
  std::string client_id_;

  std::unique_ptr<SyncStartupTracker> sync_startup_tracker_;

  base::WeakPtrFactory<DiceTurnSyncOnHelper> weak_pointer_factory_;
  DISALLOW_COPY_AND_ASSIGN(DiceTurnSyncOnHelper);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SIGNIN_DICE_TURN_SYNC_ON_HELPER_H_
