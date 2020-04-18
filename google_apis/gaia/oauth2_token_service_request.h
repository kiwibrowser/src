// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_GAIA_OAUTH2_TOKEN_SERVICE_REQUEST_H_
#define GOOGLE_APIS_GAIA_OAUTH2_TOKEN_SERVICE_REQUEST_H_

#include <memory>
#include <set>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/single_thread_task_runner.h"
#include "google_apis/gaia/oauth2_token_service.h"

// OAuth2TokenServiceRequest represents an asynchronous request to an
// OAuth2TokenService that may live in another thread.
//
// An OAuth2TokenServiceRequest can be created and started from any thread.
class OAuth2TokenServiceRequest : public OAuth2TokenService::Request {
 public:
  class Core;

  // Interface for providing an OAuth2TokenService.
  //
  // Ref-counted so that OAuth2TokenServiceRequest can ensure this object isn't
  // destroyed out from under the token service task runner thread.  Because
  // OAuth2TokenServiceRequest has a reference, implementations of
  // TokenServiceProvider must be capable of being destroyed on the same thread
  // on which the OAuth2TokenServiceRequest was created.
  class TokenServiceProvider
      : public base::RefCountedThreadSafe<TokenServiceProvider> {
   public:
    TokenServiceProvider();

    // Returns the task runner on which the token service lives.
    //
    // This method may be called from any thread.
    virtual scoped_refptr<base::SingleThreadTaskRunner>
        GetTokenServiceTaskRunner() = 0;

    // Returns a pointer to a token service.
    //
    // Caller does not own the token service and must not delete it.  The token
    // service must outlive all instances of OAuth2TokenServiceRequest.
    //
    // This method may only be called from the task runner returned by
    // |GetTokenServiceTaskRunner|.
    virtual OAuth2TokenService* GetTokenService() = 0;

   protected:
    friend class base::RefCountedThreadSafe<TokenServiceProvider>;
    virtual ~TokenServiceProvider();
  };

  // Creates and starts an access token request for |account_id| and |scopes|.
  //
  // |provider| is used to get the OAuth2TokenService.
  //
  // |account_id| must not be empty.
  //
  // |scopes| must not be empty.
  //
  // |consumer| will be invoked in the same thread that invoked CreateAndStart
  // and must outlive the returned request object.  Destroying the request
  // object ensure that |consumer| will not be called.  However, the actual
  // network activities may not be canceled and the cache in OAuth2TokenService
  // may be populated with the fetched results.
  static std::unique_ptr<OAuth2TokenServiceRequest> CreateAndStart(
      const scoped_refptr<TokenServiceProvider>& provider,
      const std::string& account_id,
      const OAuth2TokenService::ScopeSet& scopes,
      OAuth2TokenService::Consumer* consumer);

  // Invalidates |access_token| for |account_id| and |scopes|.
  //
  // |provider| is used to get the OAuth2TokenService.
  //
  // |account_id| must not be empty.
  //
  // |scopes| must not be empty.
  static void InvalidateToken(
      const scoped_refptr<TokenServiceProvider>& provider,
      const std::string& account_id,
      const OAuth2TokenService::ScopeSet& scopes,
      const std::string& access_token);

  ~OAuth2TokenServiceRequest() override;

  // OAuth2TokenService::Request.
  std::string GetAccountId() const override;

 private:
  OAuth2TokenServiceRequest(const std::string& account_id);

  void StartWithCore(const scoped_refptr<Core>& core);

  const std::string account_id_;
  scoped_refptr<Core> core_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(OAuth2TokenServiceRequest);
};

#endif  // GOOGLE_APIS_GAIA_OAUTH2_TOKEN_SERVICE_REQUEST_H_
