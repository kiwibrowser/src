// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/api/identity/identity_api.h"

#include <set>
#include <string>

#include "base/guid.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "extensions/common/manifest_handlers/oauth2_manifest_handler.h"
#include "extensions/shell/browser/shell_oauth2_token_service.h"
#include "extensions/shell/common/api/identity.h"
#include "google_apis/gaia/gaia_auth_util.h"

namespace extensions {
namespace shell {

namespace {
const char kIdentityApiId[] = "identity_api";
const char kErrorNoUserAccount[] = "No user account.";
const char kErrorNoRefreshToken[] = "No refresh token.";
const char kErrorNoScopesInManifest[] = "No scopes in manifest.";
const char kErrorUserPermissionRequired[] =
    "User permission required but not available in app_shell";
}  // namespace

IdentityAPI::IdentityAPI(content::BrowserContext* context)
    : device_id_(base::GenerateGUID()) {
}

IdentityAPI::~IdentityAPI() {
}

// static
IdentityAPI* IdentityAPI::Get(content::BrowserContext* context) {
  return BrowserContextKeyedAPIFactory<IdentityAPI>::Get(context);
}

// static
BrowserContextKeyedAPIFactory<IdentityAPI>* IdentityAPI::GetFactoryInstance() {
  static base::LazyInstance<
      BrowserContextKeyedAPIFactory<IdentityAPI>>::DestructorAtExit factory =
      LAZY_INSTANCE_INITIALIZER;
  return factory.Pointer();
}

///////////////////////////////////////////////////////////////////////////////

IdentityGetAuthTokenFunction::IdentityGetAuthTokenFunction()
    : OAuth2TokenService::Consumer(kIdentityApiId) {
}

IdentityGetAuthTokenFunction::~IdentityGetAuthTokenFunction() {
}

void IdentityGetAuthTokenFunction::SetMintTokenFlowForTesting(
    OAuth2MintTokenFlow* flow) {
  mint_token_flow_.reset(flow);
}

ExtensionFunction::ResponseAction IdentityGetAuthTokenFunction::Run() {
  std::unique_ptr<api::identity::GetAuthToken::Params> params(
      api::identity::GetAuthToken::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  ShellOAuth2TokenService* service = ShellOAuth2TokenService::GetInstance();
  std::string account_id = service->AccountId();
  if (account_id.empty())
    return RespondNow(Error(kErrorNoUserAccount));

  if (!service->RefreshTokenIsAvailable(account_id))
    return RespondNow(Error(kErrorNoRefreshToken));

  // Verify that we have scopes.
  const OAuth2Info& oauth2_info = OAuth2Info::GetOAuth2Info(extension());
  if (oauth2_info.scopes.empty())
    return RespondNow(Error(kErrorNoScopesInManifest));

  // Balanced in OnGetTokenFailure() and in the OAuth2MintTokenFlow callbacks.
  AddRef();

  // First, fetch a logged-in-user access token for the Chrome project client ID
  // and client secret. This token is used later to get a second access token
  // that will be returned to the app.
  std::set<std::string> no_scopes;
  access_token_request_ =
      service->StartRequest(service->AccountId(), no_scopes, this);
  return RespondLater();
}

void IdentityGetAuthTokenFunction::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  // Tests may override the mint token flow.
  if (!mint_token_flow_) {
    const OAuth2Info& oauth2_info = OAuth2Info::GetOAuth2Info(extension());
    DCHECK(!oauth2_info.scopes.empty());

    mint_token_flow_.reset(new OAuth2MintTokenFlow(
        this,
        OAuth2MintTokenFlow::Parameters(
            extension()->id(),
            oauth2_info.client_id,
            oauth2_info.scopes,
            IdentityAPI::Get(browser_context())->device_id(),
            OAuth2MintTokenFlow::MODE_MINT_TOKEN_FORCE)));
  }

  // Use the logging-in-user access token to mint an access token for this app.
  mint_token_flow_->Start(
      content::BrowserContext::GetDefaultStoragePartition(browser_context())->
          GetURLRequestContext(),
      access_token);
}

void IdentityGetAuthTokenFunction::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  Respond(Error(error.ToString()));
  Release();  // Balanced in Run().
}

void IdentityGetAuthTokenFunction::OnMintTokenSuccess(
    const std::string& access_token,
    int time_to_live) {
  Respond(OneArgument(std::make_unique<base::Value>(access_token)));
  Release();  // Balanced in Run().
}

void IdentityGetAuthTokenFunction::OnIssueAdviceSuccess(
    const IssueAdviceInfo& issue_advice) {
  Respond(Error(kErrorUserPermissionRequired));
  Release();  // Balanced in Run().
}

void IdentityGetAuthTokenFunction::OnMintTokenFailure(
    const GoogleServiceAuthError& error) {
  Respond(Error(error.ToString()));
  Release();  // Balanced in Run().
}

///////////////////////////////////////////////////////////////////////////////

IdentityRemoveCachedAuthTokenFunction::IdentityRemoveCachedAuthTokenFunction() {
}

IdentityRemoveCachedAuthTokenFunction::
    ~IdentityRemoveCachedAuthTokenFunction() {
}

ExtensionFunction::ResponseAction IdentityRemoveCachedAuthTokenFunction::Run() {
  std::unique_ptr<api::identity::RemoveCachedAuthToken::Params> params(
      api::identity::RemoveCachedAuthToken::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  // This stub identity API does not maintain a token cache, so there is nothing
  // to remove.
  return RespondNow(NoArguments());
}

}  // namespace shell
}  // namespace extensions
