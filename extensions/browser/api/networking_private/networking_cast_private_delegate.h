// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_NETWORKING_PRIVATE_NETWORKING_CAST_PRIVATE_DELEGATE_H_
#define EXTENSIONS_BROWSER_API_NETWORKING_PRIVATE_NETWORKING_CAST_PRIVATE_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "extensions/common/api/networking_private.h"

namespace extensions {

// Delegate interface that provides crypto methods needed to verify cast
// certificates and encrypt data using public key derived from the verified
// certificate.
// TODO(tbarzic): This is to be used during migration of
//     networkingPrivate.verify* methods to networking.castPrivate API to share
//     verification logic shared between networkingPrivate and
//     networking.castPrivate API. When the deprecated networkingPrivate methods
//     are removed, this interface should be removed, too.
class NetworkingCastPrivateDelegate {
 public:
  virtual ~NetworkingCastPrivateDelegate() {}

  using FailureCallback = base::Callback<void(const std::string& error)>;
  using VerifiedCallback = base::Callback<void(bool is_valid)>;
  using DataCallback = base::Callback<void(const std::string& encrypted_data)>;

  // API independent wrapper around cast device verification properties.
  class Credentials {
   public:
    Credentials(const std::string& certificate,
                const std::vector<std::string>& intermediate_certificates,
                const std::string& signed_data,
                const std::string& device_ssid,
                const std::string& device_serial,
                const std::string& device_bssid,
                const std::string& public_key,
                const std::string& nonce);
    ~Credentials();

    const std::string& certificate() const { return certificate_; }
    const std::vector<std::string>& intermediate_certificates() const {
      return intermediate_certificates_;
    }
    const std::string& signed_data() const { return signed_data_; }
    const std::string& unsigned_data() const { return unsigned_data_; }
    const std::string& device_bssid() const { return device_bssid_; }
    const std::string& public_key() const { return public_key_; }

   private:
    std::string certificate_;
    std::vector<std::string> intermediate_certificates_;
    std::string signed_data_;
    std::string unsigned_data_;
    std::string device_bssid_;
    std::string public_key_;

   private:
    DISALLOW_COPY_AND_ASSIGN(Credentials);
  };

  // Verifies that data provided in |credentials| authenticates a cast device.
  virtual void VerifyDestination(std::unique_ptr<Credentials> credentials,
                                 const VerifiedCallback& success_callback,
                                 const FailureCallback& failure_callback) = 0;

  // Verifies that data provided in |credentials| authenticates a cast device.
  // If the device is verified as a cast device, it fetches credentials of the
  // network identified with |network_guid| and returns the network credentials
  // encrypted with a public key derived from |credentials|.
  virtual void VerifyAndEncryptCredentials(
      const std::string& network_guid,
      std::unique_ptr<Credentials> credentials,
      const DataCallback& encrypted_credetials_callback,
      const FailureCallback& failure_callback) = 0;

  // Verifies that data provided in |credentials| authenticates a cast device.
  // If the device is verified as a cast device, it returns |data| encrypted
  // with a public key derived from |credentials|.
  virtual void VerifyAndEncryptData(
      const std::string& data,
      std::unique_ptr<Credentials> credentials,
      const DataCallback& enrypted_data_callback,
      const FailureCallback& failure_callback) = 0;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_NETWORKING_PRIVATE_NETWORKING_CAST_PRIVATE_DELEGATE_H_
