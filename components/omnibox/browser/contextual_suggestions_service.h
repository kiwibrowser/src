// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_CONTEXTUAL_SUGGESTIONS_SERVICE_H_
#define COMPONENTS_OMNIBOX_BROWSER_CONTEXTUAL_SUGGESTIONS_SERVICE_H_

#include <memory>
#include <string>

#include "components/keyed_service/core/keyed_service.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "services/identity/public/cpp/primary_account_access_token_fetcher.h"
#include "url/gurl.h"

class OAuth2TokenService;
class SigninManagerBase;
class TemplateURLService;

// A service to fetch suggestions from a remote endpoint given a URL.
class ContextualSuggestionsService : public KeyedService {
 public:
  // |signin_manager| and |token_service| may be null but only unauthenticated
  // requests will issued.
  // |request_context|  may be null, but some services may be disabled.
  ContextualSuggestionsService(SigninManagerBase* signin_manager,
                               OAuth2TokenService* token_service,
                               net::URLRequestContextGetter* request_context);

  ~ContextualSuggestionsService() override;

  using ContextualSuggestionsCallback =
      base::OnceCallback<void(std::unique_ptr<net::URLFetcher> fetcher)>;

  // Creates a fetcher for contextual suggestions for |current_url| and passes
  // the fetcher to the provided callback.
  //
  // |current_url| may be empty, in which case the system will never use the
  // experimental suggestions service. It's possible the non-experimental
  // service may decide to offer general-purpose suggestions.
  //
  // |visit_time| is the time of the visit for the URL for which suggestions
  // should be fetched.
  //
  // |template_url_service| may be null, but some services may be disabled.
  //
  // |fetcher_delegate| is used to create a fetcher that is used to perform a
  // network request. It uses a number of signals to create the fetcher,
  // including field trial / experimental parameters, and it may pass a nullptr
  // fetcher to the callback.
  //
  // |callback| is called with the fetcher produced by the |fetcher_delegate| as
  // an argument. |callback| is called with a nullptr argument if:
  //   * The service is waiting for a previously-requested authentication token.
  //   * The authentication token had any issues.
  //
  // This method sends a request for an OAuth2 token and temporarily
  // instantiates |token_fetcher_|.
  void CreateContextualSuggestionsRequest(
      const std::string& current_url,
      const base::Time& visit_time,
      const TemplateURLService* template_url_service,
      net::URLFetcherDelegate* fetcher_delegate,
      ContextualSuggestionsCallback callback);

  // Advises the service to stop any process that creates a suggestion request.
  void StopCreatingContextualSuggestionsRequest();

  // Returns a URL representing the address of the server where the zero suggest
  // request is being sent. Does not take into account whether sending this
  // request is prohibited (e.g. in an incognito window).
  // Returns an invalid URL (i.e.: GURL::is_valid() == false) in case of an
  // error.
  //
  // |current_url| is the page the user is currently browsing and may be empty.
  //
  // Note that this method is public and is also used by ZeroSuggestProvider for
  // suggestions that do not take |current_url| into consideration.
  static GURL ContextualSuggestionsUrl(
      const std::string& current_url,
      const TemplateURLService* template_url_service);

 private:
  // Returns a URL representing the address of the server where the zero suggest
  // requests are being redirected when serving experimental suggestions.
  //
  // Returns a valid URL only if all the folowing conditions are true:
  // * The |current_url| is non-empty.
  // * The |default_provider| is Google.
  // * The user is in the ZeroSuggestRedirectToChrome field trial.
  // * The field trial provides a valid server address where the suggest request
  //   is redirected.
  // * The suggest request is over HTTPS. This avoids leaking the current page
  //   URL or personal data in unencrypted network traffic.
  // Note: these checks are in addition to CanSendUrl() on the default
  // contextual suggestion URL.
  GURL ExperimentalContextualSuggestionsUrl(
      const std::string& current_url,
      const TemplateURLService* template_url_service) const;

  // Upon succesful creation of an HTTP GET request for default contextual
  // suggestions, the |callback| function is run with the HTTP GET request as a
  // parameter.
  //
  // This function is called by CreateContextualSuggestionsRequest. See its
  // function definition for details on the parameters.
  void CreateDefaultRequest(const std::string& current_url,
                            const TemplateURLService* template_url_service,
                            net::URLFetcherDelegate* fetcher_delegate,
                            ContextualSuggestionsCallback callback);

  // Upon succesful creation of an HTTP POST request for experimental contextual
  // suggestions, the |callback| function is run with the HTTP POST request as a
  // parameter.
  //
  // This function is called by CreateContextualSuggestionsRequest. See its
  // function definition for details on the parameters.
  void CreateExperimentalRequest(const std::string& current_url,
                                 const base::Time& visit_time,
                                 const GURL& suggest_url,
                                 net::URLFetcherDelegate* fetcher_delegate,
                                 ContextualSuggestionsCallback callback);

  // Called when an access token request completes (successfully or not).
  void AccessTokenAvailable(std::unique_ptr<net::URLFetcher> fetcher,
                            ContextualSuggestionsCallback callback,
                            const GoogleServiceAuthError& error,
                            const std::string& access_token);

  net::URLRequestContextGetter* request_context_;
  SigninManagerBase* signin_manager_;
  OAuth2TokenService* token_service_;

  // Helper for fetching OAuth2 access tokens. This is non-null when an access
  // token request is currently in progress.
  std::unique_ptr<identity::PrimaryAccountAccessTokenFetcher> token_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsService);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_CONTEXTUAL_SUGGESTIONS_SERVICE_H_
