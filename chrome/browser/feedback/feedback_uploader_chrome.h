// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEEDBACK_FEEDBACK_UPLOADER_CHROME_H_
#define CHROME_BROWSER_FEEDBACK_FEEDBACK_UPLOADER_CHROME_H_

#include <string>

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "components/feedback/feedback_uploader.h"
#include "google_apis/gaia/oauth2_token_service.h"

namespace feedback {

class FeedbackUploaderChrome : public OAuth2TokenService::Consumer,
                               public FeedbackUploader {
 public:
  FeedbackUploaderChrome(
      content::BrowserContext* context,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~FeedbackUploaderChrome() override;

 private:
  // OAuth2TokenService::Consumer:
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  // feedback::FeedbackUploader:
  void StartDispatchingReport() override;
  void AppendExtraHeadersToUploadRequest(net::URLFetcher* fetcher) override;

  std::unique_ptr<OAuth2TokenService::Request> access_token_request_;

  std::string access_token_;

  DISALLOW_COPY_AND_ASSIGN(FeedbackUploaderChrome);
};

}  // namespace feedback

#endif  // CHROME_BROWSER_FEEDBACK_FEEDBACK_UPLOADER_CHROME_H_
