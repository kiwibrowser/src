// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/mac/touch_id_context.h"

#import <Foundation/Foundation.h>

#include "base/strings/sys_string_conversions.h"

namespace device {
namespace fido {
namespace mac {

namespace {
API_AVAILABLE(macosx(10.12.2))
base::ScopedCFTypeRef<SecAccessControlRef> DefaultAccessControl() {
  return base::ScopedCFTypeRef<SecAccessControlRef>(
      SecAccessControlCreateWithFlags(
          kCFAllocatorDefault, kSecAttrAccessibleWhenUnlockedThisDeviceOnly,
          kSecAccessControlPrivateKeyUsage | kSecAccessControlTouchIDAny,
          nullptr));
}
}  // namespace

TouchIdContext::TouchIdContext()
    : context_([[LAContext alloc] init]),
      access_control_(DefaultAccessControl()),
      callback_(),
      weak_ptr_factory_(this) {}

TouchIdContext::~TouchIdContext() = default;

void TouchIdContext::PromptTouchId(std::string reason, Callback callback) {
  callback_ = std::move(callback);
  auto weak_self = weak_ptr_factory_.GetWeakPtr();
  // If evaluation succeeds (i.e. user provides a fingerprint), |context_| can
  // be used for one signing operation. N.B. even in |MakeCredentialOperation|,
  // we need to perform a signature for the attestation statement, so we need
  // the sign bit there.
  [context_ evaluateAccessControl:access_control_
                        operation:LAAccessControlOperationUseKeySign
                  localizedReason:base::SysUTF8ToNSString(reason)
                            reply:^(BOOL success, NSError* error) {
                              if (!weak_self) {
                                return;
                              }
                              std::move(callback_).Run(success, error);
                            }];
}

}  // namespace mac
}  // namespace fido
}  // namespace device
