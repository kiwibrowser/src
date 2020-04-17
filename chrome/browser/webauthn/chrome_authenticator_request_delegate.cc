// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/webauthn/chrome_authenticator_request_delegate.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/permissions/attestation_permission_request.h"
#include "chrome/browser/permissions/permission_request_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webauthn/authenticator_request_dialog.h"
#include "chrome/browser/webauthn/authenticator_request_dialog_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

#if defined(OS_MACOSX)
#include "device/fido/mac/credential_metadata.h"
#endif

namespace {

// Returns true iff |relying_party_id| is listed in the
// SecurityKeyPermitAttestation policy.
bool IsWebauthnRPIDListedInEnterprisePolicy(
    content::BrowserContext* browser_context,
    const std::string& relying_party_id) {
#if defined(OS_ANDROID)
  return false;
#else
  const Profile* profile = Profile::FromBrowserContext(browser_context);
  const PrefService* prefs = profile->GetPrefs();
  const base::ListValue* permit_attestation =
      prefs->GetList(prefs::kSecurityKeyPermitAttestation);
  return std::any_of(permit_attestation->begin(), permit_attestation->end(),
                     [&relying_party_id](const base::Value& v) {
                       return v.GetString() == relying_party_id;
                     });
#endif
}

}  // namespace

#if defined(OS_MACOSX)
static const char kWebAuthnTouchIdMetadataSecretPrefName[] =
    "webauthn.touchid.metadata_secret";
#endif

// static
void ChromeAuthenticatorRequestDelegate::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
#if defined(OS_MACOSX)
  registry->RegisterStringPref(kWebAuthnTouchIdMetadataSecretPrefName,
                               std::string());
#endif
}

ChromeAuthenticatorRequestDelegate::ChromeAuthenticatorRequestDelegate(
    content::RenderFrameHost* render_frame_host)
    : render_frame_host_(render_frame_host), weak_ptr_factory_(this) {}

ChromeAuthenticatorRequestDelegate::~ChromeAuthenticatorRequestDelegate() {
  // Currently, completion of the request is indicated by //content destroying
  // this delegate.
  if (weak_dialog_model_) {
    weak_dialog_model_->OnRequestComplete();
  }

  // The dialog model may be destroyed after the OnRequestComplete call.
  if (weak_dialog_model_) {
    weak_dialog_model_->RemoveObserver(this);
    weak_dialog_model_ = nullptr;
  }
}

base::WeakPtr<ChromeAuthenticatorRequestDelegate>
ChromeAuthenticatorRequestDelegate::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

content::BrowserContext* ChromeAuthenticatorRequestDelegate::browser_context()
    const {
  return content::WebContents::FromRenderFrameHost(render_frame_host())
      ->GetBrowserContext();
}

void ChromeAuthenticatorRequestDelegate::DidStartRequest() {
#if !defined(OS_ANDROID)
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kWebAuthenticationUI)) {
    return;
  }
  auto dialog_model = std::make_unique<AuthenticatorRequestDialogModel>();
  weak_dialog_model_ = dialog_model.get();
  weak_dialog_model_->AddObserver(this);
  ShowAuthenticatorRequestDialog(
      content::WebContents::FromRenderFrameHost(render_frame_host()),
      std::move(dialog_model));
#endif
}

bool ChromeAuthenticatorRequestDelegate::ShouldPermitIndividualAttestation(
    const std::string& relying_party_id) {
  // If the RP ID is listed in the policy, signal that individual attestation is
  // permitted.
  return IsWebauthnRPIDListedInEnterprisePolicy(browser_context(),
                                                relying_party_id);
}

void ChromeAuthenticatorRequestDelegate::ShouldReturnAttestation(
    const std::string& relying_party_id,
    base::OnceCallback<void(bool)> callback) {
#if defined(OS_ANDROID)
  // Android is expected to use platform APIs for webauthn which will take care
  // of prompting.
  std::move(callback).Run(true);
#else
  if (IsWebauthnRPIDListedInEnterprisePolicy(browser_context(),
                                             relying_party_id)) {
    std::move(callback).Run(true);
    return;
  }

  // This does not use content::PermissionControllerDelegate because that only
  // works with content settings, while this permission is a non-persisted,
  // per-attested- registration consent.
  auto* permission_request_manager = PermissionRequestManager::FromWebContents(
      content::WebContents::FromRenderFrameHost(render_frame_host()));
  if (!permission_request_manager) {
    std::move(callback).Run(false);
    return;
  }

  // The created AttestationPermissionRequest deletes itself once complete.
  //
  // |callback| is called via the |MessageLoop| because otherwise the
  // permissions bubble will have focus and |AuthenticatorImpl| checks that the
  // frame still has focus before returning any results.
  permission_request_manager->AddRequest(NewAttestationPermissionRequest(
      render_frame_host()->GetLastCommittedOrigin(),
      base::BindOnce(
          [](base::OnceCallback<void(bool)> callback, bool result) {
            base::ThreadTaskRunnerHandle::Get()->PostTask(
                FROM_HERE, base::BindOnce(std::move(callback), result));
          },
          std::move(callback))));
#endif
}

bool ChromeAuthenticatorRequestDelegate::IsFocused() {
#if defined(OS_ANDROID)
  // Android is expected to use platform APIs for webauthn.
  return true;
#else
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host());
  DCHECK(web_contents);
  return web_contents->GetVisibility() == content::Visibility::VISIBLE;
#endif
}

#if defined(OS_MACOSX)
static constexpr char kTouchIdKeychainAccessGroup[] =
    "EQHXZ8M8AV.com.google.Chrome.webauthn";

namespace {

std::string TouchIdMetadataSecret(Profile* profile) {
  PrefService* prefs = profile->GetPrefs();
  std::string key = prefs->GetString(kWebAuthnTouchIdMetadataSecretPrefName);
  if (key.empty() || !base::Base64Decode(key, &key)) {
    key = device::fido::mac::CredentialMetadata::GenerateRandomSecret();
    std::string encoded_key;
    base::Base64Encode(key, &encoded_key);
    prefs->SetString(kWebAuthnTouchIdMetadataSecretPrefName, encoded_key);
  }
  return key;
}

}  // namespace

// static
content::AuthenticatorRequestClientDelegate::TouchIdAuthenticatorConfig
ChromeAuthenticatorRequestDelegate::TouchIdAuthenticatorConfigForProfile(
    Profile* profile) {
  return content::AuthenticatorRequestClientDelegate::
      TouchIdAuthenticatorConfig{kTouchIdKeychainAccessGroup,
                                 TouchIdMetadataSecret(profile)};
}

base::Optional<
    content::AuthenticatorRequestClientDelegate::TouchIdAuthenticatorConfig>
ChromeAuthenticatorRequestDelegate::GetTouchIdAuthenticatorConfig() const {
  return TouchIdAuthenticatorConfigForProfile(
      Profile::FromBrowserContext(browser_context()));
}
#endif

void ChromeAuthenticatorRequestDelegate::OnModelDestroyed() {
  DCHECK(weak_dialog_model_);
  weak_dialog_model_ = nullptr;
}
