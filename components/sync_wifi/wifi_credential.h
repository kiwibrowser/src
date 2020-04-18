// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_WIFI_WIFI_CREDENTIAL_H_
#define COMPONENTS_SYNC_WIFI_WIFI_CREDENTIAL_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "components/sync_wifi/wifi_security_class.h"

namespace base {
class DictionaryValue;
}

namespace sync_wifi {

// A container to hold the information required to locate and connect
// to a WiFi network.
class WifiCredential final {  // final because the class is copyable
 public:
  // Per IEEE 802.11-2012 (Sec 8.4.2.2), the only requirement on SSIDs
  // is that they are between 0 and 32 bytes in length
  // (inclusive). There are no restrictions on the values of those
  // bytes. The SSID is not, e.g., required to be encoded as UTF-8.
  using SsidBytes = std::vector<uint8_t>;
  using CredentialSet =
      std::set<WifiCredential,
               bool (*)(const WifiCredential& a, const WifiCredential& b)>;

  WifiCredential(const WifiCredential& other);
  ~WifiCredential();

  // Creates a WifiCredential with the given |ssid|, |security_class|,
  // and |passphrase|. No assumptions are made about the input
  // encoding of |ssid|. |passphrase|, however, must be valid
  // UTF-8. Returns NULL if the parameters are invalid.
  static std::unique_ptr<WifiCredential> Create(
      const SsidBytes& ssid,
      WifiSecurityClass security_class,
      const std::string& passphrase);

  const SsidBytes& ssid() const { return ssid_; }
  WifiSecurityClass security_class() const { return security_class_; }
  const std::string& passphrase() const { return passphrase_; }

  // Returns a dictionary which represents this WifiCredential as ONC
  // properties. The resulting dictionary can be used, e.g, to
  // configure a new network using
  // chromeos::NetworkConfigurationHandler::CreateConfiguration. Due
  // to limitations in ONC, this operation fails if ssid() is not
  // valid UTF-8. In case of failure, returns a scoped_ptr with value
  // nullptr.
  std::unique_ptr<base::DictionaryValue> ToOncProperties() const;

  // Returns a string representation of the credential, for debugging
  // purposes. The string will not include the credential's passphrase.
  std::string ToString() const;

  // Returns true if credential |a| comes before credential |b|.
  static bool IsLessThan(const WifiCredential& a, const WifiCredential& b);

  // Returns an empty set of WifiCredentials, with the IsLessThan
  // ordering function plumbed in.
  static CredentialSet MakeSet();

  // Returns |ssid| as an SsidBytes instance. This convenience
  // function simplifies some tests, which need to instantiate
  // SsidBytes from string literals.
  static SsidBytes MakeSsidBytesForTest(const std::string& ssid);

 private:
  // Constructs a credential with the given |ssid|, |security_class|,
  // and |passphrase|.
  WifiCredential(const SsidBytes& ssid,
                 WifiSecurityClass security_class,
                 const std::string& passphrase);

  // The WiFi network's SSID.
  const SsidBytes ssid_;
  // The WiFi network's security class (e.g. WEP, PSK).
  const WifiSecurityClass security_class_;
  // The passphrase for connecting to the network.
  const std::string passphrase_;
};

}  // namespace sync_wifi

#endif  // COMPONENTS_SYNC_WIFI_WIFI_CREDENTIAL_H_
