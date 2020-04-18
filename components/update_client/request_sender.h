// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UPDATE_CLIENT_REQUEST_SENDER_H_
#define COMPONENTS_UPDATE_CLIENT_REQUEST_SENDER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace client_update_protocol {
class Ecdsa;
}

namespace net {
class URLFetcher;
}

namespace update_client {

class Configurator;

// Sends a request to one of the urls provided. The class implements a chain
// of responsibility design pattern, where the urls are tried in the order they
// are specified, until the request to one of them succeeds or all have failed.
// CUP signing is optional.
class RequestSender : public net::URLFetcherDelegate {
 public:
  // If |error| is 0, then the response is provided in the |response| parameter.
  // |retry_after_sec| contains the value of the X-Retry-After response header,
  // when the response was received from a cryptographically secure URL. The
  // range for this value is [-1, 86400]. If |retry_after_sec| is -1 it means
  // that the header could not be found, or trusted, or had an invalid value.
  // The upper bound represents a delay of one day.
  using RequestSenderCallback = base::OnceCallback<
      void(int error, const std::string& response, int retry_after_sec)>;

  // This value is chosen not to conflict with network errors defined by
  // net/base/net_error_list.h. The callers don't have to handle this error in
  // any meaningful way, but this value may be reported in UMA stats, therefore
  // avoiding collisions with known network errors is desirable.
  enum : int { kErrorResponseNotTrusted = -10000 };

  explicit RequestSender(scoped_refptr<Configurator> config);
  ~RequestSender() override;

  // |use_signing| enables CUP signing of protocol messages exchanged using
  // this class. |is_foreground| controls the presence and the value for the
  // X-GoogleUpdate-Interactvity header serialized in the protocol request.
  // If this optional parameter is set, the values of "fg" or "bg" are sent
  // for true or false values of this parameter. Otherwise the header is not
  // sent at all.
  void Send(const std::vector<GURL>& urls,
            const std::map<std::string, std::string>& request_extra_headers,
            const std::string& request_body,
            bool use_signing,
            RequestSenderCallback request_sender_callback);

 private:
  // Combines the |url| and |query_params| parameters.
  static GURL BuildUpdateUrl(const GURL& url, const std::string& query_params);

  // Decodes and returns the public key used by CUP.
  static std::string GetKey(const char* key_bytes_base64);

  // Returns the string value of a header of the server response or an empty
  // string if the header is not available.
  static std::string GetStringHeaderValue(const net::URLFetcher* source,
                                          const char* header_name);

  // Returns the integral value of a header of the server response or -1 if
  // if the header is not available or a conversion error has occured.
  static int64_t GetInt64HeaderValue(const net::URLFetcher* source,
                                     const char* header_name);

  // Overrides for URLFetcherDelegate.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Implements the error handling and url fallback mechanism.
  void SendInternal();

  // Called when SendInternal completes. |response_body| and |response_etag|
  // contain the body and the etag associated with the HTTP response.
  void SendInternalComplete(int error,
                            const std::string& response_body,
                            const std::string& response_etag,
                            int retry_after_sec);

  // Helper function to handle a non-continuable error in Send.
  void HandleSendError(int error, int retry_after_sec);

  base::ThreadChecker thread_checker_;

  const scoped_refptr<Configurator> config_;

  std::vector<GURL> urls_;
  std::map<std::string, std::string> request_extra_headers_;
  std::string request_body_;
  bool use_signing_;  // True if CUP signing is used.
  RequestSenderCallback request_sender_callback_;

  std::string public_key_;
  std::vector<GURL>::const_iterator cur_url_;
  std::unique_ptr<net::URLFetcher> url_fetcher_;
  std::unique_ptr<client_update_protocol::Ecdsa> signer_;

  DISALLOW_COPY_AND_ASSIGN(RequestSender);
};

}  // namespace update_client

#endif  // COMPONENTS_UPDATE_CLIENT_REQUEST_SENDER_H_
