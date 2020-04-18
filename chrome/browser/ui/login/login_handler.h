// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LOGIN_LOGIN_HANDLER_H_
#define CHROME_BROWSER_UI_LOGIN_LOGIN_HANDLER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/synchronization/lock.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/login_delegate.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/resource_request_info.h"

class GURL;
class LoginInterstitialDelegate;
class PopunderPreventer;

namespace content {
class NotificationRegistrar;
class WebContents;
}  // namespace content

namespace net {
class AuthChallengeInfo;
}  // namespace net

// This is the base implementation for the OS-specific classes that route
// authentication info to the net::URLRequest that needs it. These functions
// must be implemented in a thread safe manner.
class LoginHandler : public content::LoginDelegate,
                     public password_manager::LoginModelObserver,
                     public content::NotificationObserver {
 public:
  // The purpose of this struct is to enforce that BuildViewImpl receives either
  // both the login model and the observed form, or none. That is a bit spoiled
  // by the fact that the model is a pointer to LoginModel, as opposed to a
  // reference. Having it as a reference would go against the style guide, which
  // forbids non-const references in arguments, presumably also inside passed
  // structs, because the guide's rationale still applies. Therefore at least
  // the constructor DCHECKs that |login_model| is not null.
  struct LoginModelData {
    LoginModelData(password_manager::LoginModel* login_model,
                   const autofill::PasswordForm& observed_form);

    password_manager::LoginModel* const model;
    const autofill::PasswordForm& form;
  };

  LoginHandler(
      net::AuthChallengeInfo* auth_info,
      content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
      LoginAuthRequiredCallback auth_required_callback);

  // Builds the platform specific LoginHandler. Used from within
  // CreateLoginPrompt() which creates tasks.
  static scoped_refptr<LoginHandler> Create(
      net::AuthChallengeInfo* auth_info,
      content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
      LoginAuthRequiredCallback auth_required_callback);

  void SetInterstitialDelegate(
      const base::WeakPtr<LoginInterstitialDelegate> delegate) {
    interstitial_delegate_ = delegate;
  }

  // LoginDelegate implementation:
  void OnRequestCancelled() override;

  // Use this to build a view with password manager support. |password_manager|
  // must not be null.
  void BuildViewWithPasswordManager(
      const base::string16& authority,
      const base::string16& explanation,
      password_manager::PasswordManager* password_manager,
      const autofill::PasswordForm& observed_form);

  // Use this to build a view without password manager support.
  void BuildViewWithoutPasswordManager(const base::string16& authority,
                                       const base::string16& explanation);

  // Returns the WebContents that needs authentication.
  content::WebContents* GetWebContentsForLogin() const;

  // Returns the PasswordManager for the web contents that needs login.
  password_manager::PasswordManager* GetPasswordManagerForLogin();

  // Resend the request with authentication credentials.
  // This function can be called from either thread.
  void SetAuth(const base::string16& username, const base::string16& password);

  // Display the error page without asking for credentials again.
  // This function can be called from either thread.
  void CancelAuth();

  // Implements the content::NotificationObserver interface.
  // Listens for AUTH_SUPPLIED and AUTH_CANCELLED notifications from other
  // LoginHandlers so that this LoginHandler has the chance to dismiss itself
  // if it was waiting for the same authentication.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Who/where/what asked for the authentication.
  const net::AuthChallengeInfo* auth_info() const { return auth_info_.get(); }

  // Returns whether authentication had been handled (SetAuth or CancelAuth).
  bool WasAuthHandled() const;

 protected:
  ~LoginHandler() override;

  // Implement this to initialize the underlying platform specific view. If
  // |login_model_data| is not null, the contained LoginModel and PasswordForm
  // can be used to register the view.
  virtual void BuildViewImpl(const base::string16& authority,
                             const base::string16& explanation,
                             LoginModelData* login_model_data) = 0;

  // Sets |model_data.model| as |login_model_| and registers |this| as an
  // observer for |model_data.form|-related events.
  void SetModel(LoginModelData model_data);

  // Clears |login_model_| and removes |this| as an observer.
  void ResetModel();

  // Notify observers that authentication is needed.
  void NotifyAuthNeeded();

  // Performs necessary cleanup before deletion.
  void ReleaseSoon();

  // Closes the native dialog.
  virtual void CloseDialog() = 0;

 private:
  friend scoped_refptr<LoginHandler> CreateLoginPrompt(
      net::AuthChallengeInfo* auth_info,
      content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
      bool is_request_for_main_frame,
      const GURL& url,
      LoginAuthRequiredCallback auth_required_callback);
  FRIEND_TEST_ALL_PREFIXES(LoginHandlerTest, DialogStringsAndRealm);

  // Starts observing notifications from other LoginHandlers.
  void AddObservers();

  // Stops observing notifications from other LoginHandlers.
  void RemoveObservers();

  // Notify observers that authentication is supplied.
  void NotifyAuthSupplied(const base::string16& username,
                          const base::string16& password);

  // Notify observers that authentication is cancelled.
  void NotifyAuthCancelled(bool cancel_navigation);

  // Marks authentication as handled and returns the previous handled
  // state.
  bool TestAndSetAuthHandled();

  // Calls SetAuth from the IO loop.
  void SetAuthDeferred(const base::string16& username,
                       const base::string16& password);

  // Cancels the auth. If |cancel_navigation| is true, the existing login
  // interstitial (if any) is closed and the pending navigation is cancelled.
  void DoCancelAuth(bool cancel_navigation);

  // Calls CancelAuth from the IO loop.
  void CancelAuthDeferred();

  // Closes the view_contents from the UI loop.
  void CloseContentsDeferred();

  // Get the signon_realm under which this auth info should be stored.
  //
  // The format of the signon_realm for proxy auth is:
  //     proxy-host:proxy-port/auth-realm
  // The format of the signon_realm for server auth is:
  //     url-scheme://url-host[:url-port]/auth-realm
  //
  // Be careful when changing this function, since you could make existing
  // saved logins un-retrievable.
  static std::string GetSignonRealm(const GURL& url,
                                    const net::AuthChallengeInfo& auth_info);

  // Helper to create a PasswordForm for PasswordManager to start looking for
  // saved credentials.
  static autofill::PasswordForm MakeInputForPasswordManager(
      const GURL& url,
      const net::AuthChallengeInfo& auth_info);

  static void GetDialogStrings(const GURL& request_url,
                               const net::AuthChallengeInfo& auth_info,
                               base::string16* authority,
                               base::string16* explanation);

  static void ShowLoginPrompt(const GURL& request_url,
                              net::AuthChallengeInfo* auth_info,
                              LoginHandler* handler);

  // This callback is run on the UI thread and creates a constrained window with
  // a LoginView to prompt the user. If the prompt is triggered because of a
  // cross origin navigation in the main frame, a blank interstitial is first
  // created which in turn creates the LoginView. Otherwise, a LoginView is
  // created directly in this callback. In both cases, the response will be sent
  // to LoginHandler, which then routes it to the net::URLRequest on the I/O
  // thread.
  static void LoginDialogCallback(const GURL& request_url,
                                  net::AuthChallengeInfo* auth_info,
                                  LoginHandler* handler,
                                  bool is_main_frame);

  // True if we've handled auth (SetAuth or CancelAuth has been called).
  bool handled_auth_;
  mutable base::Lock handled_auth_lock_;

  // Who/where/what asked for the authentication.
  scoped_refptr<net::AuthChallengeInfo> auth_info_;

  // The PasswordForm sent to the PasswordManager. This is so we can refer to it
  // when later notifying the password manager if the credentials were accepted
  // or rejected.  This should only be accessed on the UI loop.
  autofill::PasswordForm password_form_;

  // Points to the password manager owned by the WebContents requesting auth.
  // This should only be accessed on the UI loop.
  password_manager::PasswordManager* password_manager_;

  // Cached from the net::URLRequest, in case it goes NULL on us.
  content::ResourceRequestInfo::WebContentsGetter web_contents_getter_;

  // If not null, points to a model we need to notify of our own destruction
  // so it doesn't try and access this when its too late.
  password_manager::LoginModel* login_model_;

  // Observes other login handlers so this login handler can respond.
  // This is only accessed on the UI thread.
  std::unique_ptr<content::NotificationRegistrar> registrar_;

  LoginAuthRequiredCallback auth_required_callback_;

  base::WeakPtr<LoginInterstitialDelegate> interstitial_delegate_;

  // Default to |false|. Must be set to |true| anytime the login handler is
  // shown.
  bool has_shown_login_handler_;

  // ReleaseSoon() should be called exactly once to destroy the object.
  bool release_soon_has_been_called_;

#if !defined(OS_ANDROID)
  std::unique_ptr<PopunderPreventer> popunder_preventer_;
#endif
};

// Details to provide the content::NotificationObserver.  Used by the automation
// proxy for testing.
class LoginNotificationDetails {
 public:
  explicit LoginNotificationDetails(LoginHandler* handler)
      : handler_(handler) {}
  LoginHandler* handler() const { return handler_; }

 private:
  LoginNotificationDetails() {}

  LoginHandler* handler_;  // Where to send the response.

  DISALLOW_COPY_AND_ASSIGN(LoginNotificationDetails);
};

// Details to provide the NotificationObserver.  Used by the automation proxy
// for testing and by other LoginHandlers to dismiss themselves when an
// identical auth is supplied.
class AuthSuppliedLoginNotificationDetails : public LoginNotificationDetails {
 public:
  AuthSuppliedLoginNotificationDetails(LoginHandler* handler,
                                       const base::string16& username,
                                       const base::string16& password)
      : LoginNotificationDetails(handler),
        username_(username),
        password_(password) {}
  const base::string16& username() const { return username_; }
  const base::string16& password() const { return password_; }

 private:
  // The username that was used for the authentication.
  const base::string16 username_;

  // The password that was used for the authentication.
  const base::string16 password_;

  DISALLOW_COPY_AND_ASSIGN(AuthSuppliedLoginNotificationDetails);
};

// Prompts the user for their username and password.  This is designed to
// be called on the background (I/O) thread, in response to
// net::URLRequest::Delegate::OnAuthRequired.  The prompt will be created
// on the main UI thread via a call to UI loop's InvokeLater, and will send the
// credentials back to the net::URLRequest on the calling thread.
// A LoginHandler object (which lives on the calling thread) is returned,
// which can be used to set or cancel authentication programmatically.  The
// caller must invoke OnRequestCancelled() on this LoginHandler before
// destroying the net::URLRequest.
scoped_refptr<LoginHandler> CreateLoginPrompt(
    net::AuthChallengeInfo* auth_info,
    content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
    bool is_main_frame,
    const GURL& url,
    LoginAuthRequiredCallback auth_required_callback);

#endif  // CHROME_BROWSER_UI_LOGIN_LOGIN_HANDLER_H_
