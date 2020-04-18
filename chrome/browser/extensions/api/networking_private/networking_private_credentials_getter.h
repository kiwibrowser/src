// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_NETWORKING_PRIVATE_NETWORKING_PRIVATE_CREDENTIALS_GETTER_H_
#define CHROME_BROWSER_EXTENSIONS_API_NETWORKING_PRIVATE_NETWORKING_PRIVATE_CREDENTIALS_GETTER_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"

namespace extensions {

// NetworkingPrivateCredentialsGetter gets plain-text WiFi credentials from the
// system and encrypts it with public key.
class NetworkingPrivateCredentialsGetter {
 public:
  typedef base::Callback<void(const std::string& key_data,
                              const std::string& error)> CredentialsCallback;

  static NetworkingPrivateCredentialsGetter* Create();

  NetworkingPrivateCredentialsGetter() {}

  virtual ~NetworkingPrivateCredentialsGetter() {}

  // Starts getting credentials. The credentials and, in case of an error, the
  // error code are returned using |callback|.
  // The NetworkingPrivateCredentialsGetter implementation should ensure that
  // the credentials request can be successfully processed even if |this| gets
  // deleted immediately after calling this method.
  // Note that there are no guarantees about the thread on which |callback| is
  // run. The caller should make sure that the result is processed on the right
  // thread.
  virtual void Start(const std::string& network_guid,
                     const std::string& public_key,
                     const CredentialsCallback& callback) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkingPrivateCredentialsGetter);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_NETWORKING_PRIVATE_NETWORKING_PRIVATE_CREDENTIALS_GETTER_H_
