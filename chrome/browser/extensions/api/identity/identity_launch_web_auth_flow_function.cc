// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/identity/identity_launch_web_auth_flow_function.h"

#include <memory>

#include "chrome/browser/extensions/api/identity/identity_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/identity.h"

namespace extensions {

namespace {

static const char kChromiumDomainRedirectUrlPattern[] =
    "https://%s.chromiumapp.org/";

}  // namespace

namespace identity = api::identity;

IdentityLaunchWebAuthFlowFunction::IdentityLaunchWebAuthFlowFunction() {}

IdentityLaunchWebAuthFlowFunction::~IdentityLaunchWebAuthFlowFunction() {
  if (auth_flow_)
    auth_flow_.release()->DetachDelegateAndDelete();
}

bool IdentityLaunchWebAuthFlowFunction::RunAsync() {
  if (GetProfile()->IsOffTheRecord()) {
    error_ = identity_constants::kOffTheRecord;
    return false;
  }

  std::unique_ptr<identity::LaunchWebAuthFlow::Params> params(
      identity::LaunchWebAuthFlow::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  GURL auth_url(params->details.url);
  WebAuthFlow::Mode mode =
      params->details.interactive && *params->details.interactive ?
      WebAuthFlow::INTERACTIVE : WebAuthFlow::SILENT;

  // Set up acceptable target URLs. (Does not include chrome-extension
  // scheme for this version of the API.)
  InitFinalRedirectURLPrefix(extension()->id());

  AddRef();  // Balanced in OnAuthFlowSuccess/Failure.

  auth_flow_.reset(new WebAuthFlow(this, GetProfile(), auth_url, mode));
  auth_flow_->Start();
  return true;
}

void IdentityLaunchWebAuthFlowFunction::InitFinalRedirectURLPrefixForTest(
    const std::string& extension_id) {
  InitFinalRedirectURLPrefix(extension_id);
}

void IdentityLaunchWebAuthFlowFunction::InitFinalRedirectURLPrefix(
    const std::string& extension_id) {
  if (final_url_prefix_.is_empty()) {
    final_url_prefix_ = GURL(base::StringPrintf(
        kChromiumDomainRedirectUrlPattern, extension_id.c_str()));
  }
}

void IdentityLaunchWebAuthFlowFunction::OnAuthFlowFailure(
    WebAuthFlow::Failure failure) {
  switch (failure) {
    case WebAuthFlow::WINDOW_CLOSED:
      error_ = identity_constants::kUserRejected;
      break;
    case WebAuthFlow::INTERACTION_REQUIRED:
      error_ = identity_constants::kInteractionRequired;
      break;
    case WebAuthFlow::LOAD_FAILED:
      error_ = identity_constants::kPageLoadFailure;
      break;
    default:
      NOTREACHED() << "Unexpected error from web auth flow: " << failure;
      error_ = identity_constants::kInvalidRedirect;
      break;
  }
  SendResponse(false);
  if (auth_flow_)
    auth_flow_.release()->DetachDelegateAndDelete();
  Release();  // Balanced in RunAsync.
}

void IdentityLaunchWebAuthFlowFunction::OnAuthFlowURLChange(
    const GURL& redirect_url) {
  if (redirect_url.GetWithEmptyPath() == final_url_prefix_) {
    SetResult(std::make_unique<base::Value>(redirect_url.spec()));
    SendResponse(true);
    if (auth_flow_)
      auth_flow_.release()->DetachDelegateAndDelete();
    Release();  // Balanced in RunAsync.
  }
}

}  // namespace extensions
