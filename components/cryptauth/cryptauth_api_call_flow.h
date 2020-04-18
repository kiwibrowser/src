// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_CRYPTAUTH_API_CALL_FLOW_H_
#define COMPONENTS_CRYPTAUTH_CRYPTAUTH_API_CALL_FLOW_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "google_apis/gaia/oauth2_api_call_flow.h"

namespace cryptauth {

// Google API call flow implementation underlying all CryptAuth API calls.
// CryptAuth is a Google service that manages authorization and cryptographic
// credentials for users' devices (eg. public keys).
class CryptAuthApiCallFlow : public OAuth2ApiCallFlow {
 public:
  typedef base::Callback<void(const std::string& serialized_response)>
      ResultCallback;
  typedef base::Callback<void(const std::string& error_message)> ErrorCallback;

  CryptAuthApiCallFlow();
  ~CryptAuthApiCallFlow() override;

  // Starts the API call.
  //   request_url: The URL endpoint of the API request.
  //   context: The URL context used to make the request.
  //   access_token: The access token for whom to make the to make the request.
  //   serialized_request: A serialized proto containing the request data.
  //   result_callback: Called when the flow completes successfully with a
  //       serialized response proto.
  //   error_callback: Called when the flow completes with an error.
  virtual void Start(const GURL& request_url,
                     net::URLRequestContextGetter* context,
                     const std::string& access_token,
                     const std::string& serialized_request,
                     const ResultCallback& result_callback,
                     const ErrorCallback& error_callback);

  void SetPartialNetworkTrafficAnnotation(
      const net::PartialNetworkTrafficAnnotationTag&
          partial_traffic_annotation) {
    partial_network_annotation_.reset(
        new net::PartialNetworkTrafficAnnotationTag(
            partial_traffic_annotation));
  }

 protected:
  // Reduce the visibility of OAuth2ApiCallFlow::Start() to avoid exposing
  // overloaded methods.
  using OAuth2ApiCallFlow::Start;

  // google_apis::OAuth2ApiCallFlow:
  GURL CreateApiCallUrl() override;
  std::string CreateApiCallBody() override;
  std::string CreateApiCallBodyContentType() override;
  net::URLFetcher::RequestType GetRequestTypeForBody(
      const std::string& body) override;
  void ProcessApiCallSuccess(const net::URLFetcher* source) override;
  void ProcessApiCallFailure(const net::URLFetcher* source) override;
  net::PartialNetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag()
      override;

 private:
  // The URL of the CryptAuth endpoint serving the request.
  GURL request_url_;

  // Serialized request message proto that will be sent in the API request.
  std::string serialized_request_;

  // Callback invoked with the serialized response message proto when the flow
  // completes successfully.
  ResultCallback result_callback_;

  // Callback invoked with an error message when the flow fails.
  ErrorCallback error_callback_;

  std::unique_ptr<net::PartialNetworkTrafficAnnotationTag>
      partial_network_annotation_;

  DISALLOW_COPY_AND_ASSIGN(CryptAuthApiCallFlow);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_CRYPTAUTH_API_CALL_FLOW_H_
