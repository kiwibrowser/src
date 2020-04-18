// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_DRIVE_REQUEST_SENDER_H_
#define GOOGLE_APIS_DRIVE_REQUEST_SENDER_H_

#include <memory>
#include <set>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "google_apis/drive/drive_api_error_codes.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace base {
class SequencedTaskRunner;
}

namespace net {
class URLRequestContextGetter;
}

namespace google_apis {

class AuthenticatedRequestInterface;
class AuthServiceInterface;

// Helper class that sends requests implementing
// AuthenticatedRequestInterface and handles retries and authentication.
class RequestSender {
 public:
  // |auth_service| is used for fetching OAuth tokens. It'll be owned by
  // this RequestSender.
  //
  // |url_request_context_getter| is the context used to perform network
  // requests from this RequestSender.
  //
  // |blocking_task_runner| is used for running blocking operation, e.g.,
  // parsing JSON response from the server.
  //
  // |custom_user_agent| will be used for the User-Agent header in HTTP
  // requests issued through the request sender if the value is not empty.
  RequestSender(
      AuthServiceInterface* auth_service,
      net::URLRequestContextGetter* url_request_context_getter,
      const scoped_refptr<base::SequencedTaskRunner>& blocking_task_runner,
      const std::string& custom_user_agent,
      const net::NetworkTrafficAnnotationTag& traffic_annotation);
  ~RequestSender();

  AuthServiceInterface* auth_service() { return auth_service_.get(); }

  net::URLRequestContextGetter* url_request_context_getter() const {
    return url_request_context_getter_.get();
  }

  base::SequencedTaskRunner* blocking_task_runner() const {
    return blocking_task_runner_.get();
  }

  // Starts a request implementing the AuthenticatedRequestInterface
  // interface, and makes the request retry upon authentication failures by
  // calling back to RetryRequest.
  //
  // Returns a closure to cancel the request. The closure cancels the request
  // if it is in-flight, and does nothing if it is already terminated.
  base::Closure StartRequestWithAuthRetry(
      std::unique_ptr<AuthenticatedRequestInterface> request);

  // Notifies to this RequestSender that |request| has finished.
  // TODO(kinaba): refactor the life time management and make this at private.
  void RequestFinished(AuthenticatedRequestInterface* request);

  // Returns traffic annotation tag asssigned to this object.
  const net::NetworkTrafficAnnotationTag& get_traffic_annotation_tag() const {
    return traffic_annotation_;
  }

 private:
  base::Closure StartRequestWithAuthRetryInternal(
      AuthenticatedRequestInterface* request);

  // Called when the access token is fetched.
  void OnAccessTokenFetched(
      const base::WeakPtr<AuthenticatedRequestInterface>& request,
      DriveApiErrorCode error,
      const std::string& access_token);

  // Clears any authentication token and retries the request, which forces
  // an authentication token refresh.
  void RetryRequest(AuthenticatedRequestInterface* request);

  // Cancels the request. Used for implementing the returned closure of
  // StartRequestWithAuthRetry.
  void CancelRequest(
      const base::WeakPtr<AuthenticatedRequestInterface>& request);

  std::unique_ptr<AuthServiceInterface> auth_service_;
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;

  std::set<std::unique_ptr<AuthenticatedRequestInterface>> in_flight_requests_;
  const std::string custom_user_agent_;

  base::ThreadChecker thread_checker_;

  const net::NetworkTrafficAnnotationTag traffic_annotation_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<RequestSender> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RequestSender);
};

}  // namespace google_apis

#endif  // GOOGLE_APIS_DRIVE_REQUEST_SENDER_H_
