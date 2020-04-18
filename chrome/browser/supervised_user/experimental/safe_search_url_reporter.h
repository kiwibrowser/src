// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUPERVISED_USER_EXPERIMENTAL_SAFE_SEARCH_URL_REPORTER_H_
#define CHROME_BROWSER_SUPERVISED_USER_EXPERIMENTAL_SAFE_SEARCH_URL_REPORTER_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "url/gurl.h"

class GURL;
class Profile;

namespace network {
class SharedURLLoaderFactory;
}

class SafeSearchURLReporter : public OAuth2TokenService::Consumer {
 public:
  using SuccessCallback = base::OnceCallback<void(bool)>;

  SafeSearchURLReporter(
      OAuth2TokenService* oauth2_token_service,
      const std::string& account_id,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SafeSearchURLReporter() override;

  static std::unique_ptr<SafeSearchURLReporter> CreateWithProfile(
      Profile* profile);

  void ReportUrl(const GURL& url, SuccessCallback callback);

 private:
  struct Report;
  using ReportList = std::list<std::unique_ptr<Report>>;

  // OAuth2TokenService::Consumer implementation:
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  void OnSimpleLoaderComplete(ReportList::iterator it,
                              std::unique_ptr<std::string> response_body);

  // Requests an access token, which is the first thing we need. This is where
  // we restart when the returned access token has expired.
  void StartFetching(Report* Report);

  void DispatchResult(ReportList::iterator it, bool success);

  OAuth2TokenService* oauth2_token_service_;
  std::string account_id_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  ReportList reports_;

  DISALLOW_COPY_AND_ASSIGN(SafeSearchURLReporter);
};

#endif  // CHROME_BROWSER_SUPERVISED_USER_EXPERIMENTAL_SAFE_SEARCH_URL_REPORTER_H_
