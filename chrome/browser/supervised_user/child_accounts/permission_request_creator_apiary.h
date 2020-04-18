// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUPERVISED_USER_CHILD_ACCOUNTS_PERMISSION_REQUEST_CREATOR_APIARY_H_
#define CHROME_BROWSER_SUPERVISED_USER_CHILD_ACCOUNTS_PERMISSION_REQUEST_CREATOR_APIARY_H_

#include <list>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "chrome/browser/supervised_user/permission_request_creator.h"
#include "google_apis/gaia/oauth2_token_service.h"

class GURL;
class Profile;

namespace base {
class Time;
}

namespace network {
class SharedURLLoaderFactory;
}

class PermissionRequestCreatorApiary : public PermissionRequestCreator,
                                       public OAuth2TokenService::Consumer {
 public:
  PermissionRequestCreatorApiary(
      OAuth2TokenService* oauth2_token_service,
      const std::string& account_id,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~PermissionRequestCreatorApiary() override;

  static std::unique_ptr<PermissionRequestCreator> CreateWithProfile(
      Profile* profile);

  // PermissionRequestCreator implementation:
  bool IsEnabled() const override;
  void CreateURLAccessRequest(const GURL& url_requested,
                              SuccessCallback callback) override;
  void CreateExtensionInstallRequest(const std::string& id,
                                     SuccessCallback callback) override;
  void CreateExtensionUpdateRequest(const std::string& id,
                                    SuccessCallback callback) override;

 private:
  friend class PermissionRequestCreatorApiaryTest;

  struct Request;
  using RequestList = std::list<std::unique_ptr<Request>>;

  // OAuth2TokenService::Consumer implementation:
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  void OnSimpleLoaderComplete(RequestList::iterator it,
                              std::unique_ptr<std::string> response_body);

  GURL GetApiUrl() const;
  std::string GetApiScope() const;

  void CreateRequest(const std::string& request_type,
                     const std::string& object_ref,
                     SuccessCallback callback);

  // Requests an access token, which is the first thing we need. This is where
  // we restart when the returned access token has expired.
  void StartFetching(Request* request);

  void DispatchResult(RequestList::iterator it, bool success);

  OAuth2TokenService* oauth2_token_service_;
  std::string account_id_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  bool retry_on_network_change_;

  RequestList requests_;

  DISALLOW_COPY_AND_ASSIGN(PermissionRequestCreatorApiary);
};

#endif  // CHROME_BROWSER_SUPERVISED_USER_CHILD_ACCOUNTS_PERMISSION_REQUEST_CREATOR_APIARY_H_
