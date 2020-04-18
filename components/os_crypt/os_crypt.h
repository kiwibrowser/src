// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OS_CRYPT_OS_CRYPT_H_
#define COMPONENTS_OS_CRYPT_OS_CRYPT_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string16.h"
#include "build/build_config.h"

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(UNIT_TEST)
class KeyStorageLinux;
#endif  // defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(UNIT_TEST)

namespace os_crypt {
struct Config;
}

// The OSCrypt class gives access to simple encryption and decryption of
// strings. Note that on Mac, access to the system Keychain is required and
// these calls can block the current thread to collect user input. The same is
// true for Linux, if a password management tool is available.
class OSCrypt {
 public:
#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  // Set the configuration of OSCrypt.
  static void SetConfig(std::unique_ptr<os_crypt::Config> config);

  // Returns true iff the real secret key (not hardcoded one) is available.
  static bool IsEncryptionAvailable();
#endif  // defined(OS_LINUX) && !defined(OS_CHROMEOS)

  // Encrypt a string16. The output (second argument) is really an array of
  // bytes, but we're passing it back as a std::string.
  static bool EncryptString16(const base::string16& plaintext,
                              std::string* ciphertext);

  // Decrypt an array of bytes obtained with EncryptString16 back into a
  // string16. Note that the input (first argument) is a std::string, so you
  // need to first get your (binary) data into a string.
  static bool DecryptString16(const std::string& ciphertext,
                              base::string16* plaintext);

  // Encrypt a string.
  static bool EncryptString(const std::string& plaintext,
                            std::string* ciphertext);

  // Decrypt an array of bytes obtained with EnctryptString back into a string.
  // Note that the input (first argument) is a std::string, so you need to first
  // get your (binary) data into a string.
  static bool DecryptString(const std::string& ciphertext,
                            std::string* plaintext);

#if defined(OS_MACOSX)
  // For unit testing purposes we instruct the Encryptor to use a mock Keychain
  // on the Mac. The default is to use the real Keychain.
  static void UseMockKeychain(bool use_mock);
#endif

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(OSCrypt);
};

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(UNIT_TEST)
// For unit testing purposes, inject methods to be used.
// |get_key_storage_mock| provides the desired |KeyStorage| implementation.
// If the provider returns |nullptr|, a hardcoded password will be used.
// |get_password_v11_mock| provides a password to derive the encryption key from
// If one parameter is |nullptr|, the function will be not be replaced.
// If all parameters are |nullptr|, the real implementation is restored.
void UseMockKeyStorageForTesting(
    std::unique_ptr<KeyStorageLinux> (*get_key_storage_mock)(),
    std::string* (*get_password_v11_mock)());

// Clears any caching and most lazy initialisations performed by the production
// code. Should be used after any test which required a password.
void ClearCacheForTesting();
#endif  // defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(UNIT_TEST)

#endif  // COMPONENTS_OS_CRYPT_OS_CRYPT_H_
