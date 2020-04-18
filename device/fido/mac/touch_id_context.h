// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_MAC_TOUCH_ID_CONTEXT_H_
#define DEVICE_FIDO_MAC_TOUCH_ID_CONTEXT_H_

#import <LocalAuthentication/LocalAuthentication.h>
#import <Security/Security.h>

#include "base/callback.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace device {
namespace fido {
namespace mac {

// TouchIdContext wraps a macOS Touch ID consent prompt for signing with a
// secure enclave key.
class API_AVAILABLE(macosx(10.12.2)) TouchIdContext {
 public:
  // The callback is invoked when the Touch ID prompt completes. It receives a
  // boolean indicating success and an autoreleased NSError if the prompt was
  // denied or failed.
  using Callback = base::OnceCallback<void(bool, NSError*)>;

  TouchIdContext();
  ~TouchIdContext();

  // PromptTouchId displays a Touch ID consent prompt with the provided reason
  // string to the user. On completion or error, the provided callback is
  // invoked, unless the TouchIdContext instance has been destroyed in the
  // meantime (in which case nothing happens).
  void PromptTouchId(std::string reason, Callback callback);

  // authentication_context returns the LAContext used for the Touch ID prompt.
  LAContext* authentication_context() const { return context_; }

  // access_control returns a reference to the SecAccessControl object that was
  // evaluated/authorized in the Touch ID prompt.
  SecAccessControlRef access_control() const { return access_control_; }

 private:
  base::scoped_nsobject<LAContext> context_;
  base::ScopedCFTypeRef<SecAccessControlRef> access_control_;
  Callback callback_;
  base::WeakPtrFactory<TouchIdContext> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TouchIdContext);
};

}  // namespace mac
}  // namespace fido
}  // namespace device

#endif  // DEVICE_FIDO_MAC_TOUCH_ID_CONTEXT_H_
