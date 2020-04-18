// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SYNC_ONE_CLICK_SIGNIN_SYNC_STARTER_H_
#define CHROME_BROWSER_UI_SYNC_ONE_CLICK_SIGNIN_SYNC_STARTER_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/sync/profile_signin_confirmation_helper.h"
#include "chrome/browser/ui/webui/signin/login_ui_service.h"
#include "components/signin/core/browser/signin_tracker.h"

class Browser;

namespace browser_sync {
class ProfileSyncService;
}  // namespace browser_sync

namespace syncer {
class SyncSetupInProgressHandle;
}  // namespace syncer

// Waits for successful sign-in notification from the signin manager and then
// starts the sync machine.  Instances of this class delete themselves once
// the job is done.
class OneClickSigninSyncStarter : public SigninTracker::Observer,
                                  public BrowserListObserver,
                                  public LoginUIService::Observer {
 public:
  enum ProfileMode {
    // Attempts to sign the user in |profile_|. Note that if the account to be
    // signed in is a managed account, then a profile confirmation dialog is
    // shown and the user has the possibility to create a new profile before
    // signing in.
    CURRENT_PROFILE,

    // Creates a new profile and signs the user in this new profile.
    NEW_PROFILE
  };

  enum StartSyncMode {
    // Starts the process of signing the user in with the SigninManager, and
    // once completed automatically starts sync with all data types enabled.
    SYNC_WITH_DEFAULT_SETTINGS,

    // Starts the process of signing the user in with the SigninManager, and
    // once completed shows an inline confirmation UI for sync settings. If the
    // user dismisses the confirmation UI, sync will start immediately. If the
    // user clicks the settings link, Chrome will reidrect to the sync settings
    // page.
    CONFIRM_SYNC_SETTINGS_FIRST,

    // Starts the process of signing the user in with the SigninManager, and
    // once completed redirects the user to the settings page to allow them
    // to configure which data types to sync before sync is enabled.
    CONFIGURE_SYNC_FIRST,

    // The process should be aborted because the undo button has been pressed.
    UNDO_SYNC
  };

  enum ConfirmationRequired {
    // No need to display a "post-signin" confirmation bubble (for example, if
    // the user was doing a re-auth flow).
    NO_CONFIRMATION,

    // Signin flow redirected outside of trusted domains, so ask the user to
    // confirm before signing in.
    CONFIRM_UNTRUSTED_SIGNIN,

    // Display a confirmation after signing in.
    CONFIRM_AFTER_SIGNIN
  };

  // Result of the sync setup.
  enum SyncSetupResult {
    SYNC_SETUP_SUCCESS,
    SYNC_SETUP_FAILURE
  };

  using Callback = base::Callback<void(SyncSetupResult)>;

  // |profile| must not be NULL, however |browser| can be. When using the
  // OneClickSigninSyncStarter from a browser, provide both.
  // If |display_confirmation| is true, the user will be prompted to confirm the
  // signin before signin completes.
  // |callback| is always executed before OneClickSigninSyncStarter is deleted.
  // It can be empty.
  OneClickSigninSyncStarter(Profile* profile,
                            Browser* browser,
                            const std::string& gaia_id,
                            const std::string& email,
                            const std::string& password,
                            const std::string& refresh_token,
                            signin_metrics::AccessPoint signin_access_point,
                            signin_metrics::Reason signin_reason,
                            ProfileMode profile_mode,
                            StartSyncMode start_mode,
                            ConfirmationRequired display_confirmation,
                            Callback callback);

  // BrowserListObserver override.
  void OnBrowserRemoved(Browser* browser) override;

  // If the |browser| argument is non-null, returns the pointer directly.
  // Otherwise creates a new browser for the given profile on the given
  // desktop, adds an empty tab and makes sure the browser is visible.
  static Browser* EnsureBrowser(Browser* browser, Profile* profile);

 protected:
  ~OneClickSigninSyncStarter() override;

  // Overridden from tests.
  virtual void ShowSyncSetupSettingsSubpage();

 private:
  friend class OneClickSigninSyncStarterTest;
  FRIEND_TEST_ALL_PREFIXES(OneClickSigninSyncStarterTest, CallbackSigninFailed);
  FRIEND_TEST_ALL_PREFIXES(OneClickSigninSyncStarterTest, CallbackNull);

  // Initializes the internals of the OneClickSigninSyncStarter object. Can also
  // be used to re-initialize the object to refer to a newly created profile.
  void Initialize(Profile* profile, Browser* browser);

  // SigninTracker::Observer override.
  void SigninFailed(const GoogleServiceAuthError& error) override;
  void SigninSuccess() override;
  void AccountAddedToCookie(const GoogleServiceAuthError& error) override;

  // LoginUIService::Observer override.
  void OnSyncConfirmationUIClosed(
      LoginUIService::SyncConfirmationUIClosedResult result) override;

  // User input handler for the signin confirmation dialog.
  class SigninDialogDelegate
    : public ui::ProfileSigninConfirmationDelegate {
   public:
    SigninDialogDelegate(
        base::WeakPtr<OneClickSigninSyncStarter> sync_starter);
    ~SigninDialogDelegate() override;
    void OnCancelSignin() override;
    void OnContinueSignin() override;
    void OnSigninWithNewProfile() override;

   private:
    base::WeakPtr<OneClickSigninSyncStarter> sync_starter_;
  };
  friend class SigninDialogDelegate;

  // Callback invoked once policy registration is complete. If registration
  // fails, |dm_token| and |client_id| will be empty.
  void OnRegisteredForPolicy(const std::string& dm_token,
                             const std::string& client_id);

  // Callback invoked when a policy fetch request has completed. |success| is
  // true if policy was successfully fetched.
  void OnPolicyFetchComplete(bool success);

  // Called to create a new profile, which is then signed in with the
  // in-progress auth credentials currently stored in this object.
  void CreateNewSignedInProfile();

  // Copies the sign-in credentials to |new_profile| and starts syncing in
  // |new_profile|.
  void CopyCredentialsToNewProfileAndFinishSignin(Profile* new_profile);

  // Helper function that loads policy with the cached |dm_token_| and
  // |client_id|, then completes the signin process.
  void LoadPolicyWithCachedCredentials();

  // Callback invoked once a profile is created, so we can complete the
  // credentials transfer, load policy, and open the first window.
  void CompleteInitForNewProfile(Profile* profile,
                                 Profile::CreateStatus status);

  // Cancels the in-progress signin for this profile.
  void CancelSigninAndDelete();

  // Callback invoked to check whether the user needs policy or if a
  // confirmation is required (in which case we have to prompt the user first).
  void ConfirmSignin(ProfileMode profile_mode, const std::string& oauth_token);

  // Displays confirmation UI to the user if confirmation_required_ ==
  // CONFIRM_UNTRUSTED_SIGNIN, otherwise completes the pending signin process.
  void ConfirmAndSignin();

  // Callback invoked once the user has responded to the signin confirmation UI.
  // If response == UNDO_SYNC, the signin is cancelled, otherwise the pending
  // signin is completed.
  void UntrustedSigninConfirmed(StartSyncMode response);

  // GetProfileSyncService returns non-NULL pointer if sync is enabled.
  // There is a scenario when when ProfileSyncService discovers that sync is
  // disabled during setup. In this case GetProfileSyncService will return NULL,
  // but we still need to call PSS::SetSetupInProgress(false). For this purpose
  // call FinishProfileSyncServiceSetup() function.
  browser_sync::ProfileSyncService* GetProfileSyncService();

  // Finishes the setup of the profile sync service.
  void FinishProfileSyncServiceSetup();

  // Shows the post-signin confirmation bubble. If |custom_message| is empty,
  // the default "You are signed in" message is displayed.
  void DisplayFinalConfirmationBubble(const base::string16& custom_message);

  // Displays the sync confirmation modal dialog.
  void DisplayModalSyncConfirmationWindow();

  Profile* profile_;
  Browser* browser_;
  signin_metrics::AccessPoint signin_access_point_;
  signin_metrics::Reason signin_reason_;
  std::unique_ptr<SigninTracker> signin_tracker_;
  StartSyncMode start_mode_;
  ConfirmationRequired confirmation_required_;

  // Callback executed when sync setup succeeds or fails.
  Callback sync_setup_completed_callback_;

  // Policy credentials we keep while determining whether to create
  // a new profile for an enterprise user or not.
  std::string dm_token_;
  std::string client_id_;

  // This only cares about the first AccountAddedToCookie event. Since
  // SigninTracker always expects an observer, this object will just disregard
  // following AccountAddedToCookie calls triggered by account reconciliation.
  bool first_account_added_to_cookie_;

  // Prevents Sync from running until configuration is complete.
  std::unique_ptr<syncer::SyncSetupInProgressHandle> sync_blocker_;

  base::WeakPtrFactory<OneClickSigninSyncStarter> weak_pointer_factory_;

  DISALLOW_COPY_AND_ASSIGN(OneClickSigninSyncStarter);
};


#endif  // CHROME_BROWSER_UI_SYNC_ONE_CLICK_SIGNIN_SYNC_STARTER_H_
