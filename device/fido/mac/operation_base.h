// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_MAC_OPERATION_BASE_H_
#define DEVICE_FIDO_MAC_OPERATION_BASE_H_

#import <Foundation/Foundation.h>
#import <Security/Security.h>

#include "base/bind.h"
#include "base/callback.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/macros.h"
#include "base/strings/sys_string_conversions.h"
#include "device/fido/mac/operation.h"
#include "device/fido/mac/touch_id_context.h"

namespace device {
namespace fido {
namespace mac {

// OperationBase abstracts behavior common to both concrete Operations,
// |MakeCredentialOperation| and |GetAssertionOperation|.
template <class Request, class Response>
class API_AVAILABLE(macosx(10.12.2)) OperationBase : public Operation {
 public:
  using Callback = base::OnceCallback<void(CtapDeviceResponseCode,
                                           base::Optional<Response>)>;

  OperationBase(Request request,
                std::string profile_id,
                std::string keychain_access_group,
                Callback callback)
      : request_(std::move(request)),
        profile_id_(std::move(profile_id)),
        keychain_access_group_(std::move(keychain_access_group)),
        callback_(std::move(callback)),
        touch_id_context_(std::make_unique<TouchIdContext>()) {}
  ~OperationBase() override = default;

 protected:
  // PromptTouchId triggers a Touch ID consent dialog with the given reason
  // string. Subclasses implement the PromptTouchIdDone callback to receive the
  // result.
  void PromptTouchId(std::string reason) {
    // The callback passed to TouchIdContext::Prompt will not fire if the
    // TouchIdContext itself has been deleted. Since that it is owned by this
    // class, there is no need to bind the callback to a weak ref here.
    touch_id_context_->PromptTouchId(
        std::move(reason), base::BindOnce(&OperationBase::PromptTouchIdDone,
                                          base::Unretained(this)));
  }

  // Callback for |PromptTouchId|. Any NSError that gets passed is autoreleased.
  virtual void PromptTouchIdDone(bool success, NSError* err) = 0;

  // Subclasses override RpId to return the RP ID from the type-specific
  // request.
  virtual const std::string& RpId() const = 0;

  LAContext* authentication_context() const {
    return touch_id_context_->authentication_context();
  }
  SecAccessControlRef access_control() const {
    return touch_id_context_->access_control();
  }

  // DefaultKeychainQuery returns a default keychain query dictionary that has
  // the keychain item class, profile ID and RP ID filled out (but not the
  // credential ID). More fields can be set on the return value to refine the
  // query.
  base::ScopedCFTypeRef<CFMutableDictionaryRef> DefaultKeychainQuery() const {
    base::ScopedCFTypeRef<CFMutableDictionaryRef> query(
        CFDictionaryCreateMutable(kCFAllocatorDefault, 0, nullptr, nullptr));
    CFDictionarySetValue(query, kSecClass, kSecClassKey);
    CFDictionarySetValue(query, kSecAttrAccessGroup,
                         base::SysUTF8ToNSString(keychain_access_group_));
    CFDictionarySetValue(query, kSecAttrLabel, base::SysUTF8ToNSString(RpId()));
    CFDictionarySetValue(query, kSecAttrApplicationTag,
                         base::SysUTF8ToNSString(profile_id_));
    return query;
  }

  const Request& request() const { return request_; }
  Callback& callback() { return callback_; }

 private:
  Request request_;
  std::string profile_id_;
  std::string keychain_access_group_;
  Callback callback_;

  std::unique_ptr<TouchIdContext> touch_id_context_;
};

}  // namespace mac
}  // namespace fido
}  // namespace device

#endif  // DEVICE_FIDO_MAC_OPERATION_BASE_H_
