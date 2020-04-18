// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_MAC_KEYCHAIN_H_
#define DEVICE_FIDO_MAC_KEYCHAIN_H_

#import <Foundation/Foundation.h>
#import <Security/Security.h>

#include "base/mac/scoped_cftyperef.h"
#include "base/macros.h"
#include "base/no_destructor.h"

namespace device {
namespace fido {
namespace mac {

// Keychain wraps some operations from the macOS Security framework to work with
// keys and keychain items.
//
// The Touch ID authenticator creates keychain items in the "iOS-style"
// keychain, which scopes item access based on the application-identifer or
// keychain-access-group entitlements, and therefore requires code signing with
// a real Apple developer ID. We therefore group these function here, so they
// can be mocked out in testing.
class API_AVAILABLE(macosx(10.12.2)) Keychain {
 public:
  static const Keychain& GetInstance();

  // KeyCreateRandomKey wraps the |SecKeyCreateRandomKey| function.
  virtual base::ScopedCFTypeRef<SecKeyRef> KeyCreateRandomKey(
      CFDictionaryRef params,
      CFErrorRef* error) const;
  // KeyCreateSignature wraps the |SecKeyCreateSignature| function.
  virtual base::ScopedCFTypeRef<CFDataRef> KeyCreateSignature(
      SecKeyRef key,
      SecKeyAlgorithm algorithm,
      CFDataRef data,
      CFErrorRef* error) const;
  // KeyCopyPublicKey wraps the |SecKeyCopyPublicKey| function.
  virtual base::ScopedCFTypeRef<SecKeyRef> KeyCopyPublicKey(
      SecKeyRef key) const;

  // ItemCopyMatching wraps the |SecItemCopyMatching| function.
  virtual OSStatus ItemCopyMatching(CFDictionaryRef query,
                                    CFTypeRef* result) const;
  // ItemDelete wraps the |SecItemDelete| function.
  virtual OSStatus ItemDelete(CFDictionaryRef query) const;

 private:
  friend class base::NoDestructor<Keychain>;
  Keychain();

  DISALLOW_COPY_AND_ASSIGN(Keychain);
};

}  // namespace mac
}  // namespace fido
}  // namespace device

#endif  // DEVICE_FIDO_MAC_KEYCHAIN_H_
