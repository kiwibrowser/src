// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_API_IDENTITY_IDENTITY_API_H_
#define EXTENSIONS_SHELL_BROWSER_API_IDENTITY_IDENTITY_API_H_

#include <string>

#include "base/macros.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/extension_function.h"
#include "google_apis/gaia/oauth2_mint_token_flow.h"
#include "google_apis/gaia/oauth2_token_service.h"

namespace extensions {
namespace shell {

// Storage for data used across identity function invocations.
class IdentityAPI : public BrowserContextKeyedAPI {
 public:
  explicit IdentityAPI(content::BrowserContext* context);
  ~IdentityAPI() override;

  static IdentityAPI* Get(content::BrowserContext* context);

  const std::string& device_id() const { return device_id_; }

  // BrowserContextKeyedAPI:
  static BrowserContextKeyedAPIFactory<IdentityAPI>* GetFactoryInstance();
  static const char* service_name() { return "IdentityAPI"; }

 private:
  friend class BrowserContextKeyedAPIFactory<IdentityAPI>;

  // A GUID identifying this device.
  // TODO(jamescook): Make this GUID stable across runs of the app, perhaps by
  // storing it in a pref.
  const std::string device_id_;
};

// Returns an OAuth2 access token for a user. See the IDL file for
// documentation.
class IdentityGetAuthTokenFunction : public UIThreadExtensionFunction,
                                     public OAuth2TokenService::Consumer,
                                     public OAuth2MintTokenFlow::Delegate {
 public:
  DECLARE_EXTENSION_FUNCTION("identity.getAuthToken", UNKNOWN);

  IdentityGetAuthTokenFunction();

  // Takes ownership.
  void SetMintTokenFlowForTesting(OAuth2MintTokenFlow* flow);

 protected:
  ~IdentityGetAuthTokenFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  // OAuth2TokenService::Consumer:
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  // OAuth2MintTokenFlow::Delegate:
  void OnMintTokenSuccess(const std::string& access_token,
                          int time_to_live) override;
  void OnIssueAdviceSuccess(const IssueAdviceInfo& issue_advice) override;
  void OnMintTokenFailure(const GoogleServiceAuthError& error) override;

 private:
  // A pending token fetch request to get a login-scoped access token for the
  // current user for the Chrome project id.
  std::unique_ptr<OAuth2TokenService::Request> access_token_request_;

  // A request for an access token for the current app and its scopes.
  std::unique_ptr<OAuth2MintTokenFlow> mint_token_flow_;

  DISALLOW_COPY_AND_ASSIGN(IdentityGetAuthTokenFunction);
};

// Stub. See the IDL file for documentation.
class IdentityRemoveCachedAuthTokenFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("identity.removeCachedAuthToken", UNKNOWN)

  IdentityRemoveCachedAuthTokenFunction();

 protected:
  ~IdentityRemoveCachedAuthTokenFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(IdentityRemoveCachedAuthTokenFunction);
};

}  // namespace shell
}  // namespace extensions

#endif  // EXTENSIONS_SHELL_BROWSER_API_IDENTITY_IDENTITY_API_H_
