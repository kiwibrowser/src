// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_GAIA_OAUTH2_API_CALL_FLOW_H_
#define GOOGLE_APIS_GAIA_OAUTH2_API_CALL_FLOW_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}

// Base class for all classes that implement a flow to call OAuth2 enabled APIs,
// given an access token to the service.  This class abstracts the basic steps
// and exposes template methods for sub-classes to implement for API specific
// details.
class OAuth2ApiCallFlow : public net::URLFetcherDelegate {
 public:
  OAuth2ApiCallFlow();

  ~OAuth2ApiCallFlow() override;

  // Start the flow.
  virtual void Start(net::URLRequestContextGetter* context,
                     const std::string& access_token);

  // net::URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 protected:
  // Template methods for sub-classes.

  // Methods to help create the API request.
  virtual GURL CreateApiCallUrl() = 0;
  virtual std::string CreateApiCallBody() = 0;
  virtual std::string CreateApiCallBodyContentType();

  // Returns the request type (e.g. GET, POST) for the |body| that will be sent
  // with the request.
  virtual net::URLFetcher::RequestType GetRequestTypeForBody(
      const std::string& body);

  // Sub-classes can expose an appropriate observer interface by implementing
  // these template methods.
  // Called when the API call finished successfully.
  virtual void ProcessApiCallSuccess(const net::URLFetcher* source) = 0;
  // Called when the API call failed.
  virtual void ProcessApiCallFailure(const net::URLFetcher* source) = 0;

  virtual net::PartialNetworkTrafficAnnotationTag
  GetNetworkTrafficAnnotationTag() = 0;

 private:
  enum State {
    INITIAL,
    API_CALL_STARTED,
    API_CALL_DONE,
    ERROR_STATE
  };

  // Creates an instance of URLFetcher that does not send or save cookies.
  // Template method CreateApiCallUrl is used to get the URL.
  // Template method CreateApiCallBody is used to get the body.
  // The URLFether's method will be GET if body is empty, POST otherwise.
  std::unique_ptr<net::URLFetcher> CreateURLFetcher(
      net::URLRequestContextGetter* context,
      const std::string& access_token);

  // Helper methods to implement the state machine for the flow.
  void BeginApiCall();
  void EndApiCall(const net::URLFetcher* source);

  State state_;
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(OAuth2ApiCallFlow);
};

#endif  // GOOGLE_APIS_GAIA_OAUTH2_API_CALL_FLOW_H_
