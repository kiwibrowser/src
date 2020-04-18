// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/chrome_signin_client.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_metrics.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/signin/force_signin_verifier.h"
#include "chrome/browser/signin/local_auth.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/signin/signin_util.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/web_data_service_factory.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/pref_names.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/metrics/metrics_service.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/cookie_settings_util.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_buildflags.h"
#include "components/signin/core/browser/signin_cookie_change_subscription.h"
#include "components/signin/core/browser/signin_header_helper.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "components/signin/core/browser/signin_switches.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/gaia_urls.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_SUPERVISED_USERS)
#include "chrome/browser/supervised_user/supervised_user_constants.h"
#endif

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/net/delay_network_call.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "components/user_manager/known_user.h"
#include "components/user_manager/user_manager.h"
#else
#include "chrome/browser/ui/user_manager.h"
#endif

#if !defined(OS_ANDROID)
#include "chrome/browser/first_run/first_run.h"
#endif

ChromeSigninClient::ChromeSigninClient(
    Profile* profile,
    SigninErrorController* signin_error_controller)
    : OAuth2TokenService::Consumer("chrome_signin_client"),
      profile_(profile),
      signin_error_controller_(signin_error_controller),
      weak_ptr_factory_(this) {
  signin_error_controller_->AddObserver(this);
#if !defined(OS_CHROMEOS)
  g_browser_process->network_connection_tracker()->AddNetworkConnectionObserver(
      this);
#else
  // UserManager may not exist in unit_tests.
  if (!user_manager::UserManager::IsInitialized())
    return;

  const user_manager::User* user =
      chromeos::ProfileHelper::Get()->GetUserByProfile(profile_);
  if (!user)
    return;
  const AccountId account_id = user->GetAccountId();
  if (user_manager::known_user::GetDeviceId(account_id).empty()) {
    const std::string legacy_device_id =
        GetPrefs()->GetString(prefs::kGoogleServicesSigninScopedDeviceId);
    if (!legacy_device_id.empty()) {
      // Need to move device ID from the old location to the new one, if it has
      // not been done yet.
      user_manager::known_user::SetDeviceId(account_id, legacy_device_id);
    } else {
      user_manager::known_user::SetDeviceId(
          account_id, GenerateSigninScopedDeviceID(
                          user_manager::UserManager::Get()
                              ->IsUserNonCryptohomeDataEphemeral(account_id)));
    }
  }
  GetPrefs()->SetString(prefs::kGoogleServicesSigninScopedDeviceId,
                        std::string());
#endif
}

ChromeSigninClient::~ChromeSigninClient() {
  signin_error_controller_->RemoveObserver(this);
#if !defined(OS_CHROMEOS)
  g_browser_process->network_connection_tracker()
      ->RemoveNetworkConnectionObserver(this);
#endif
}

void ChromeSigninClient::DoFinalInit() {
  MaybeFetchSigninTokenHandle();
  VerifySyncToken();
}

// static
bool ChromeSigninClient::ProfileAllowsSigninCookies(Profile* profile) {
  content_settings::CookieSettings* cookie_settings =
      CookieSettingsFactory::GetForProfile(profile).get();
  return signin::SettingsAllowSigninCookies(cookie_settings);
}

PrefService* ChromeSigninClient::GetPrefs() { return profile_->GetPrefs(); }

scoped_refptr<TokenWebData> ChromeSigninClient::GetDatabase() {
  return WebDataServiceFactory::GetTokenWebDataForProfile(
      profile_, ServiceAccessType::EXPLICIT_ACCESS);
}

bool ChromeSigninClient::CanRevokeCredentials() {
#if defined(OS_CHROMEOS)
  // UserManager may not exist in unit_tests.
  if (user_manager::UserManager::IsInitialized() &&
      user_manager::UserManager::Get()->IsLoggedInAsSupervisedUser()) {
    // Don't allow revoking credentials for Chrome OS supervised users.
    // See http://crbug.com/332032
    LOG(ERROR) << "Attempt to revoke supervised user refresh "
               << "token detected, ignoring.";
    return false;
  }
#else
  // Don't allow revoking credentials for legacy supervised users.
  // See http://crbug.com/332032
  if (profile_->IsLegacySupervised()) {
    LOG(ERROR) << "Attempt to revoke supervised user refresh "
               << "token detected, ignoring.";
    return false;
  }
#endif
  return true;
}

std::string ChromeSigninClient::GetSigninScopedDeviceId() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableSigninScopedDeviceId)) {
    return std::string();
  }

#if !defined(OS_CHROMEOS)
  return SigninClient::GetOrCreateScopedDeviceIdPref(GetPrefs());
#else
  // UserManager may not exist in unit_tests.
  if (!user_manager::UserManager::IsInitialized())
    return std::string();

  const user_manager::User* user =
      chromeos::ProfileHelper::Get()->GetUserByProfile(profile_);
  if (!user)
    return std::string();

  const std::string signin_scoped_device_id =
      user_manager::known_user::GetDeviceId(user->GetAccountId());
  LOG_IF(ERROR, signin_scoped_device_id.empty())
      << "Device ID is not set for user.";
  return signin_scoped_device_id;
#endif
}

void ChromeSigninClient::OnSignedOut() {
  ProfileAttributesEntry* entry;
  bool has_entry = g_browser_process->profile_manager()->
      GetProfileAttributesStorage().
      GetProfileAttributesWithPath(profile_->GetPath(), &entry);

  // If sign out occurs because Sync setup was in progress and the Profile got
  // deleted, then the profile's no longer in the ProfileAttributesStorage.
  if (!has_entry)
    return;

  entry->SetLocalAuthCredentials(std::string());
  entry->SetAuthInfo(std::string(), base::string16());
  entry->SetIsSigninRequired(false);
}

net::URLRequestContextGetter* ChromeSigninClient::GetURLRequestContext() {
  return profile_->GetRequestContext();
}

bool ChromeSigninClient::ShouldMergeSigninCredentialsIntoCookieJar() {
  return !signin::IsAccountConsistencyMirrorEnabled();
}

std::string ChromeSigninClient::GetProductVersion() {
  return chrome::GetVersionString();
}

bool ChromeSigninClient::IsFirstRun() const {
#if defined(OS_ANDROID)
  return false;
#else
  return first_run::IsChromeFirstRun();
#endif
}

base::Time ChromeSigninClient::GetInstallDate() {
  return base::Time::FromTimeT(
      g_browser_process->metrics_service()->GetInstallDate());
}

bool ChromeSigninClient::AreSigninCookiesAllowed() {
  return ProfileAllowsSigninCookies(profile_);
}

void ChromeSigninClient::AddContentSettingsObserver(
    content_settings::Observer* observer) {
  HostContentSettingsMapFactory::GetForProfile(profile_)
      ->AddObserver(observer);
}

void ChromeSigninClient::RemoveContentSettingsObserver(
    content_settings::Observer* observer) {
  HostContentSettingsMapFactory::GetForProfile(profile_)
      ->RemoveObserver(observer);
}

std::unique_ptr<SigninClient::CookieChangeSubscription>
ChromeSigninClient::AddCookieChangeCallback(
    const GURL& url,
    const std::string& name,
    net::CookieChangeCallback callback) {
  scoped_refptr<net::URLRequestContextGetter> context_getter =
      profile_->GetRequestContext();
  DCHECK(context_getter.get());
  return std::make_unique<SigninCookieChangeSubscription>(
      context_getter, url, name, std::move(callback));
}

void ChromeSigninClient::OnSignedIn(const std::string& account_id,
                                    const std::string& gaia_id,
                                    const std::string& username,
                                    const std::string& password) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ProfileAttributesEntry* entry;
  if (profile_manager->GetProfileAttributesStorage().
          GetProfileAttributesWithPath(profile_->GetPath(), &entry)) {
    entry->SetAuthInfo(gaia_id, base::UTF8ToUTF16(username));
    ProfileMetrics::UpdateReportedProfilesStatistics(profile_manager);
  }
}

void ChromeSigninClient::PostSignedIn(const std::string& account_id,
                                      const std::string& username,
                                      const std::string& password) {
#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  // Don't store password hash except when lock is available for the user.
  if (!password.empty() && profiles::IsLockAvailable(profile_))
    LocalAuth::SetLocalAuthCredentials(profile_, password);
#endif
}

void ChromeSigninClient::PreSignOut(
    const base::Callback<void()>& sign_out,
    signin_metrics::ProfileSignout signout_source_metric) {
#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)

  // These sign out won't remove the policy cache, keep the window opened.
  bool keep_window_opened =
      signout_source_metric ==
          signin_metrics::GOOGLE_SERVICE_NAME_PATTERN_CHANGED ||
      signout_source_metric == signin_metrics::SERVER_FORCED_DISABLE ||
      signout_source_metric == signin_metrics::SIGNOUT_PREF_CHANGED;
  if (signin_util::IsForceSigninEnabled() && !profile_->IsSystemProfile() &&
      !profile_->IsGuestSession() && !profile_->IsSupervised() &&
      !keep_window_opened) {
    if (signout_source_metric ==
        signin_metrics::SIGNIN_PREF_CHANGED_DURING_SIGNIN) {
      // SIGNIN_PREF_CHANGED_DURING_SIGNIN will be triggered when SigninManager
      // is initialized before window opening, there is no need to close window.
      // Call OnCloseBrowsersSuccess to continue sign out and show UserManager
      // afterwards.
      should_display_user_manager_ = false;  // Don't show UserManager twice.
      OnCloseBrowsersSuccess(sign_out, signout_source_metric,
                             profile_->GetPath());
    } else {
      BrowserList::CloseAllBrowsersWithProfile(
          profile_,
          base::Bind(&ChromeSigninClient::OnCloseBrowsersSuccess,
                     base::Unretained(this), sign_out, signout_source_metric),
          base::Bind(&ChromeSigninClient::OnCloseBrowsersAborted,
                     base::Unretained(this)),
          signout_source_metric == signin_metrics::ABORT_SIGNIN ||
              signout_source_metric ==
                  signin_metrics::AUTHENTICATION_FAILED_WITH_FORCE_SIGNIN ||
              signout_source_metric == signin_metrics::TRANSFER_CREDENTIALS);
    }
  } else {
#else
  {
#endif
    SigninClient::PreSignOut(sign_out, signout_source_metric);
  }
}

void ChromeSigninClient::OnErrorChanged() {
  // Some tests don't have a ProfileManager.
  if (g_browser_process->profile_manager() == nullptr)
    return;

  ProfileAttributesEntry* entry;

  if (!g_browser_process->profile_manager()->GetProfileAttributesStorage().
          GetProfileAttributesWithPath(profile_->GetPath(), &entry)) {
    return;
  }

  entry->SetIsAuthError(signin_error_controller_->HasError());
}

void ChromeSigninClient::OnGetTokenInfoResponse(
    std::unique_ptr<base::DictionaryValue> token_info) {
  if (!token_info->HasKey("error")) {
    std::string handle;
    if (token_info->GetString("token_handle", &handle)) {
      ProfileAttributesEntry* entry = nullptr;
      bool has_entry = g_browser_process->profile_manager()->
          GetProfileAttributesStorage().
          GetProfileAttributesWithPath(profile_->GetPath(), &entry);
      DCHECK(has_entry);
      entry->SetPasswordChangeDetectionToken(handle);
    }
  }
  oauth_request_.reset();
}

void ChromeSigninClient::OnOAuthError() {
  // Ignore the failure.  It's not essential and we'll try again next time.
    oauth_request_.reset();
}

void ChromeSigninClient::OnNetworkError(int response_code) {
  // Ignore the failure.  It's not essential and we'll try again next time.
    oauth_request_.reset();
}

void ChromeSigninClient::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  // Exchange the access token for a handle that can be used for later
  // verification that the token is still valid (i.e. the password has not
  // been changed).
    if (!oauth_client_) {
        oauth_client_.reset(new gaia::GaiaOAuthClient(
            profile_->GetRequestContext()));
    }
    oauth_client_->GetTokenInfo(access_token, 3 /* retries */, this);
}

void ChromeSigninClient::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  // Ignore the failure.  It's not essential and we'll try again next time.
  oauth_request_.reset();
}

#if !defined(OS_CHROMEOS)
void ChromeSigninClient::OnConnectionChanged(
    network::mojom::ConnectionType type) {
  if (type == network::mojom::ConnectionType::CONNECTION_NONE)
    return;

  for (const base::Closure& callback : delayed_callbacks_)
    callback.Run();

  delayed_callbacks_.clear();
}
#endif

void ChromeSigninClient::DelayNetworkCall(const base::Closure& callback) {
#if defined(OS_CHROMEOS)
  chromeos::DelayNetworkCall(
      base::TimeDelta::FromMilliseconds(chromeos::kDefaultNetworkRetryDelayMS),
      callback);
  return;
#else
  // Don't bother if we don't have any kind of network connection.
  network::mojom::ConnectionType type;
  bool sync =
      g_browser_process->network_connection_tracker()->GetConnectionType(
          &type, base::BindOnce(&ChromeSigninClient::OnConnectionChanged,
                                weak_ptr_factory_.GetWeakPtr()));
  if (!sync || type == network::mojom::ConnectionType::CONNECTION_NONE) {
    // Connection type cannot be retrieved synchronously so delay the callback.
    delayed_callbacks_.push_back(callback);
  } else {
    callback.Run();
  }
#endif
}

std::unique_ptr<GaiaAuthFetcher> ChromeSigninClient::CreateGaiaAuthFetcher(
    GaiaAuthConsumer* consumer,
    const std::string& source,
    net::URLRequestContextGetter* getter) {
  return std::make_unique<GaiaAuthFetcher>(consumer, source, getter);
}

void ChromeSigninClient::VerifySyncToken() {
#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  if (signin_util::IsForceSigninEnabled())
    force_signin_verifier_ = std::make_unique<ForceSigninVerifier>(profile_);
#endif
}

void ChromeSigninClient::MaybeFetchSigninTokenHandle() {
#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  // We get a "handle" that can be used to reference the signin token on the
  // server.  We fetch this if we don't have one so that later we can check
  // it to know if the signin token to which it is attached has been revoked
  // and thus distinguish between a password mismatch due to the password
  // being changed and the user simply mis-typing it.
  if (profiles::IsLockAvailable(profile_)) {
    ProfileAttributesStorage& storage =
        g_browser_process->profile_manager()->GetProfileAttributesStorage();
    ProfileAttributesEntry* entry;
    // If we don't have a token for detecting a password change, create one.
    if (storage.GetProfileAttributesWithPath(profile_->GetPath(), &entry) &&
        entry->GetPasswordChangeDetectionToken().empty() && !oauth_request_) {
      std::string account_id = SigninManagerFactory::GetForProfile(profile_)
          ->GetAuthenticatedAccountId();
      if (!account_id.empty()) {
        ProfileOAuth2TokenService* token_service =
            ProfileOAuth2TokenServiceFactory::GetForProfile(profile_);
        OAuth2TokenService::ScopeSet scopes;
        scopes.insert(GaiaConstants::kGoogleUserInfoEmail);
        oauth_request_ = token_service->StartRequest(account_id, scopes, this);
      }
    }
  }
#endif
}

void ChromeSigninClient::AfterCredentialsCopied() {
  if (signin_util::IsForceSigninEnabled()) {
    // The signout after credential copy won't open UserManager after all
    // browser window are closed. Because the browser window will be opened for
    // the new profile soon.
    should_display_user_manager_ = false;
  }
}

void ChromeSigninClient::SetReadyForDiceMigration(bool is_ready) {
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  AccountConsistencyModeManager::GetForProfile(profile_)
      ->SetReadyForDiceMigration(is_ready);
#else
  NOTREACHED();
#endif
}

void ChromeSigninClient::OnCloseBrowsersSuccess(
    const base::Callback<void()>& sign_out,
    const signin_metrics::ProfileSignout signout_source_metric,
    const base::FilePath& profile_path) {
#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  if (signin_util::IsForceSigninEnabled() && force_signin_verifier_.get()) {
    force_signin_verifier_->Cancel();
  }
#endif
  SigninClient::PreSignOut(sign_out, signout_source_metric);

  LockForceSigninProfile(profile_path);
  // After sign out, lock the profile and show UserManager if necessary.
  if (should_display_user_manager_) {
    ShowUserManager(profile_path);
  } else {
    should_display_user_manager_ = true;
  }
}

void ChromeSigninClient::OnCloseBrowsersAborted(
    const base::FilePath& profile_path) {
  should_display_user_manager_ = true;
}

void ChromeSigninClient::LockForceSigninProfile(
    const base::FilePath& profile_path) {
  ProfileAttributesEntry* entry;
  bool has_entry =
      g_browser_process->profile_manager()
          ->GetProfileAttributesStorage()
          .GetProfileAttributesWithPath(profile_->GetPath(), &entry);
  if (!has_entry)
    return;
  entry->LockForceSigninProfile(true);
}

void ChromeSigninClient::ShowUserManager(const base::FilePath& profile_path) {
#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  UserManager::Show(profile_path,
                    profiles::USER_MANAGER_SELECT_PROFILE_NO_ACTION);
#endif
}
