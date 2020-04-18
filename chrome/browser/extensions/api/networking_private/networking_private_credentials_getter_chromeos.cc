// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/browser/extensions/api/networking_private/networking_private_credentials_getter.h"

const char kErrorNotImplemented[] = "Error.NotImplemented";

namespace extensions {

class NetworkingPrivateCredentialsGetterChromeos
    : public NetworkingPrivateCredentialsGetter {
 public:
  NetworkingPrivateCredentialsGetterChromeos() {}

  void Start(const std::string& network_guid,
             const std::string& public_key,
             const CredentialsCallback& callback) override;

 private:
  ~NetworkingPrivateCredentialsGetterChromeos() override;

  DISALLOW_COPY_AND_ASSIGN(NetworkingPrivateCredentialsGetterChromeos);
};

NetworkingPrivateCredentialsGetterChromeos::
    ~NetworkingPrivateCredentialsGetterChromeos() {
}

void NetworkingPrivateCredentialsGetterChromeos::Start(
    const std::string& network_guid,
    const std::string& public_key,
    const CredentialsCallback& callback) {
  // TODO(sheretov) add credential slurping from sync.
  callback.Run(std::string(), kErrorNotImplemented);
}

NetworkingPrivateCredentialsGetter*
NetworkingPrivateCredentialsGetter::Create() {
  return new NetworkingPrivateCredentialsGetterChromeos();
}

}  // namespace extensions
