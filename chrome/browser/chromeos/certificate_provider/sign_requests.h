// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CERTIFICATE_PROVIDER_SIGN_REQUESTS_H_
#define CHROME_BROWSER_CHROMEOS_CERTIFICATE_PROVIDER_SIGN_REQUESTS_H_

#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "net/ssl/ssl_private_key.h"

namespace chromeos {
namespace certificate_provider {

class SignRequests {
 public:
  SignRequests();
  ~SignRequests();

  // Returns the id of the new request. The returned request id is specific to
  // the given extension.
  int AddRequest(const std::string& extension_id,
                 net::SSLPrivateKey::SignCallback callback);

  // Returns false if no request with the given id for |extension_id|
  // could be found. Otherwise removes the request and sets |callback| to the
  // callback that was provided with AddRequest().
  bool RemoveRequest(const std::string& extension_id,
                     int request_id,
                     net::SSLPrivateKey::SignCallback* callback);

  // Remove all pending requests for this extension and return their
  // callbacks.
  std::vector<net::SSLPrivateKey::SignCallback> RemoveAllRequests(
      const std::string& extension_id);

 private:
  // Holds state of all sign requests to a single extension.
  struct RequestsState {
    RequestsState();
    RequestsState(RequestsState&& other);
    ~RequestsState();

    // Maps from request id to the SignCallback that must be called with the
    // signature or error.
    std::map<int, net::SSLPrivateKey::SignCallback> pending_requests;

    // The request id that will be used for the next sign request to this
    // extension.
    int next_free_id = 0;
  };

  // Contains the state of all sign requests per extension.
  std::map<std::string, RequestsState> extension_to_requests_;

  DISALLOW_COPY_AND_ASSIGN(SignRequests);
};

}  // namespace certificate_provider
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CERTIFICATE_PROVIDER_SIGN_REQUESTS_H_
