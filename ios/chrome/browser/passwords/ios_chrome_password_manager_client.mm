// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/ios_chrome_password_manager_client.h"

#include <memory>
#include <utility>

#include "components/autofill/core/common/password_form.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/log_manager.h"
#include "components/password_manager/core/browser/password_form_manager_for_ui.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/password_manager_internals_service.h"
#include "components/password_manager/core/browser/password_manager_util.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/experimental_flags.h"
#include "ios/chrome/browser/passwords/ios_chrome_password_store_factory.h"
#include "ios/chrome/browser/passwords/password_manager_internals_service_factory.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#include "net/cert/cert_status_flags.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using password_manager::PasswordFormManagerForUI;
using password_manager::PasswordManagerMetricsRecorder;
using password_manager::PasswordStore;
using password_manager::PasswordSyncState;

namespace {

const syncer::SyncService* GetSyncService(
    ios::ChromeBrowserState* browser_state) {
  return IOSChromeProfileSyncServiceFactory::GetForBrowserStateIfExists(
      browser_state);
}

const SigninManagerBase* GetSigninManager(
    ios::ChromeBrowserState* browser_state) {
  return ios::SigninManagerFactory::GetForBrowserState(browser_state);
}

}  // namespace

IOSChromePasswordManagerClient::IOSChromePasswordManagerClient(
    id<PasswordManagerClientDelegate> delegate)
    : delegate_(delegate),
      credentials_filter_(
          this,
          base::Bind(&GetSyncService, delegate_.browserState),
          base::Bind(&GetSigninManager, delegate_.browserState)),
      ukm_source_id_(0),
      helper_(this) {
  saving_passwords_enabled_.Init(
      password_manager::prefs::kCredentialsEnableService, GetPrefs());
  log_manager_ = password_manager::LogManager::Create(
      ios::PasswordManagerInternalsServiceFactory::GetForBrowserState(
          delegate_.browserState),
      base::Closure());
}

IOSChromePasswordManagerClient::~IOSChromePasswordManagerClient() = default;

PasswordSyncState IOSChromePasswordManagerClient::GetPasswordSyncState() const {
  browser_sync::ProfileSyncService* sync_service =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(
          delegate_.browserState);
  return password_manager_util::GetPasswordSyncState(sync_service);
}

bool IOSChromePasswordManagerClient::PromptUserToChooseCredentials(
    std::vector<std::unique_ptr<autofill::PasswordForm>> local_forms,
    const GURL& origin,
    const CredentialsCallback& callback) {
  NOTIMPLEMENTED();
  return false;
}

bool IOSChromePasswordManagerClient::PromptUserToSaveOrUpdatePassword(
    std::unique_ptr<PasswordFormManagerForUI> form_to_save,
    bool update_password) {
  if (form_to_save->IsBlacklisted())
    return false;

  if (update_password) {
    [delegate_ showUpdatePasswordInfoBar:std::move(form_to_save)];
  } else {
    [delegate_ showSavePasswordInfoBar:std::move(form_to_save)];
  }

  return true;
}

void IOSChromePasswordManagerClient::ShowManualFallbackForSaving(
    std::unique_ptr<password_manager::PasswordFormManagerForUI> form_to_save,
    bool has_generated_password,
    bool is_update) {
  NOTIMPLEMENTED();
}

void IOSChromePasswordManagerClient::HideManualFallbackForSaving() {
  NOTIMPLEMENTED();
}

void IOSChromePasswordManagerClient::AutomaticPasswordSave(
    std::unique_ptr<PasswordFormManagerForUI> saved_form_manager) {
  NOTIMPLEMENTED();
}

bool IOSChromePasswordManagerClient::IsIncognito() const {
  return (delegate_.browserState)->IsOffTheRecord();
}

const password_manager::PasswordManager*
IOSChromePasswordManagerClient::GetPasswordManager() const {
  return delegate_.passwordManager;
}

PrefService* IOSChromePasswordManagerClient::GetPrefs() const {
  return (delegate_.browserState)->GetPrefs();
}

PasswordStore* IOSChromePasswordManagerClient::GetPasswordStore() const {
  return IOSChromePasswordStoreFactory::GetForBrowserState(
             delegate_.browserState, ServiceAccessType::EXPLICIT_ACCESS)
      .get();
}

void IOSChromePasswordManagerClient::NotifyUserAutoSignin(
    std::vector<std::unique_ptr<autofill::PasswordForm>> local_forms,
    const GURL& origin) {
  DCHECK(!local_forms.empty());
  helper_.NotifyUserAutoSignin();
  [delegate_ showAutosigninNotification:std::move(local_forms[0])];
}

void IOSChromePasswordManagerClient::NotifyUserCouldBeAutoSignedIn(
    std::unique_ptr<autofill::PasswordForm> form) {
  helper_.NotifyUserCouldBeAutoSignedIn(std::move(form));
}

void IOSChromePasswordManagerClient::NotifySuccessfulLoginWithExistingPassword(
    const autofill::PasswordForm& form) {
  helper_.NotifySuccessfulLoginWithExistingPassword(form);
}

void IOSChromePasswordManagerClient::NotifyStorePasswordCalled() {
  helper_.NotifyStorePasswordCalled();
}

void IOSChromePasswordManagerClient::ForceSavePassword() {
  NOTIMPLEMENTED();
}

bool IOSChromePasswordManagerClient::IsSavingAndFillingEnabledForCurrentPage()
    const {
  return *saving_passwords_enabled_ && !IsIncognito() &&
         !net::IsCertStatusError(GetMainFrameCertStatus()) &&
         IsFillingEnabledForCurrentPage();
}

const GURL& IOSChromePasswordManagerClient::GetLastCommittedEntryURL() const {
  return delegate_.lastCommittedURL;
}

const password_manager::CredentialsFilter*
IOSChromePasswordManagerClient::GetStoreResultFilter() const {
  return &credentials_filter_;
}

const password_manager::LogManager*
IOSChromePasswordManagerClient::GetLogManager() const {
  return log_manager_.get();
}

ukm::SourceId IOSChromePasswordManagerClient::GetUkmSourceId() {
  // TODO(crbug.com/792662): Update this to get a shared UKM SourceId (e.g.
  // from web state), once the UKM framework provides a mechanism for that.
  if (ukm_source_url_ != delegate_.lastCommittedURL) {
    metrics_recorder_.reset();
    ukm_source_url_ = delegate_.lastCommittedURL;
    ukm_source_id_ = ukm::UkmRecorder::GetNewSourceID();
    ukm::UkmRecorder::Get()->UpdateSourceURL(ukm_source_id_, ukm_source_url_);
  }
  return ukm_source_id_;
}

PasswordManagerMetricsRecorder&
IOSChromePasswordManagerClient::GetMetricsRecorder() {
  if (!metrics_recorder_) {
    // Query source_id first, because that has the side effect of initializing
    // |ukm_source_url_|.
    ukm::SourceId source_id = GetUkmSourceId();
    metrics_recorder_.emplace(source_id, ukm_source_url_);
  }
  return metrics_recorder_.value();
}

void IOSChromePasswordManagerClient::PromptUserToEnableAutosignin() {
  // TODO(crbug.com/435048): Implement this method.
}

password_manager::PasswordManager*
IOSChromePasswordManagerClient::GetPasswordManager() {
  return delegate_.passwordManager;
}
