// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feedback/feedback_uploader_chrome.h"

#include "base/strings/stringprintf.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "content/public/browser/browser_context.h"
#include "net/url_request/url_fetcher.h"

namespace feedback {

namespace {

constexpr char kAuthenticationErrorLogMessage[] =
    "Feedback report will be sent without authentication.";

}  // namespace

FeedbackUploaderChrome::FeedbackUploaderChrome(
    content::BrowserContext* context,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : OAuth2TokenService::Consumer("feedback_uploader_chrome"),
      FeedbackUploader(context, task_runner) {}

FeedbackUploaderChrome::~FeedbackUploaderChrome() = default;

void FeedbackUploaderChrome::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  access_token_request_.reset();
  access_token_ = access_token;
  FeedbackUploader::StartDispatchingReport();
}

void FeedbackUploaderChrome::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  LOG(ERROR) << "Failed to get the access token. "
             << kAuthenticationErrorLogMessage;
  access_token_request_.reset();
  FeedbackUploader::StartDispatchingReport();
}

void FeedbackUploaderChrome::StartDispatchingReport() {
  access_token_.clear();

  Profile* profile = Profile::FromBrowserContext(context());
  DCHECK(profile);
  auto* oauth2_token_service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile);
  auto* signin_manager = SigninManagerFactory::GetForProfile(profile);
  if (oauth2_token_service && signin_manager &&
      signin_manager->IsAuthenticated()) {
    std::string account_id = signin_manager->GetAuthenticatedAccountId();
    OAuth2TokenService::ScopeSet scopes;
    scopes.insert("https://www.googleapis.com/auth/supportcontent");
    access_token_request_ =
        oauth2_token_service->StartRequest(account_id, scopes, this);
    return;
  }

  LOG(ERROR) << "Failed to request oauth access token. "
             << kAuthenticationErrorLogMessage;
  FeedbackUploader::StartDispatchingReport();
}

void FeedbackUploaderChrome::AppendExtraHeadersToUploadRequest(
    net::URLFetcher* fetcher) {
  if (access_token_.empty())
    return;

  fetcher->AddExtraRequestHeader(
      base::StringPrintf("Authorization: Bearer %s", access_token_.c_str()));
}

}  // namespace feedback
