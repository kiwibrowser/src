// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/mac/keychain.h"

namespace device {
namespace fido {
namespace mac {

// static
const Keychain& Keychain::GetInstance() {
  static const base::NoDestructor<Keychain> k;
  return *k;
}

Keychain::Keychain() = default;

base::ScopedCFTypeRef<SecKeyRef> Keychain::KeyCreateRandomKey(
    CFDictionaryRef params,
    CFErrorRef* error) const {
  return base::ScopedCFTypeRef<SecKeyRef>(SecKeyCreateRandomKey(params, error));
}

base::ScopedCFTypeRef<CFDataRef> Keychain::KeyCreateSignature(
    SecKeyRef key,
    SecKeyAlgorithm algorithm,
    CFDataRef data,
    CFErrorRef* error) const {
  return base::ScopedCFTypeRef<CFDataRef>(
      SecKeyCreateSignature(key, algorithm, data, error));
}

base::ScopedCFTypeRef<SecKeyRef> Keychain::KeyCopyPublicKey(
    SecKeyRef key) const {
  return base::ScopedCFTypeRef<SecKeyRef>(SecKeyCopyPublicKey(key));
}

OSStatus Keychain::ItemCopyMatching(CFDictionaryRef query,
                                    CFTypeRef* result) const {
  return SecItemCopyMatching(query, result);
}

OSStatus Keychain::ItemDelete(CFDictionaryRef query) const {
  return SecItemDelete(query);
}

}  // namespace mac
}  // namespace fido
}  // namespace device
