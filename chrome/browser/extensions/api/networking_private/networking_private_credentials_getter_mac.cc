// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/networking_private/networking_private_credentials_getter.h"

#include <Security/Security.h>
#include <stdint.h>

#include <memory>

#include "base/base64.h"
#include "base/bind.h"
#include "base/macros.h"
#include "chrome/browser/extensions/api/networking_private/networking_private_crypto.h"
#include "components/wifi/wifi_service.h"
#include "content/public/browser/browser_thread.h"

const char kErrorEncryption[] = "Error.Encryption";

using content::BrowserThread;

namespace extensions {

class NetworkingPrivateCredentialsGetterMac
    : public NetworkingPrivateCredentialsGetter {
 public:
  explicit NetworkingPrivateCredentialsGetterMac();

  void Start(const std::string& network_guid,
             const std::string& public_key,
             const CredentialsCallback& callback) override;

 private:
  ~NetworkingPrivateCredentialsGetterMac() override;

  DISALLOW_COPY_AND_ASSIGN(NetworkingPrivateCredentialsGetterMac);
};

NetworkingPrivateCredentialsGetterMac::NetworkingPrivateCredentialsGetterMac() {
}

NetworkingPrivateCredentialsGetterMac::
    ~NetworkingPrivateCredentialsGetterMac() {}

void NetworkingPrivateCredentialsGetterMac::Start(
    const std::string& network_guid,
    const std::string& public_key,
    const CredentialsCallback& callback) {
  std::unique_ptr<wifi::WiFiService> wifi_service(wifi::WiFiService::Create());
  wifi_service->Initialize(NULL);
  std::string key_data;
  std::string error;
  wifi_service->GetKeyFromSystem(network_guid, &key_data, &error);

  if (!error.empty()) {
    callback.Run("", error);
    return;
  }

  std::vector<uint8_t> public_key_data(public_key.begin(), public_key.end());
  std::vector<uint8_t> ciphertext;
  if (!networking_private_crypto::EncryptByteString(
          public_key_data, key_data, &ciphertext)) {
    callback.Run("", kErrorEncryption);
    return;
  }

  std::string base64_encoded_ciphertext;
  base::Base64Encode(
      base::StringPiece(reinterpret_cast<const char*>(ciphertext.data()),
                        ciphertext.size()),
      &base64_encoded_ciphertext);
  callback.Run(base64_encoded_ciphertext, "");
}

NetworkingPrivateCredentialsGetter*
NetworkingPrivateCredentialsGetter::Create() {
  return new NetworkingPrivateCredentialsGetterMac();
}

}  // namespace extensions
