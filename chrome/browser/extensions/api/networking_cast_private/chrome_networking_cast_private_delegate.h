// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_NETWORKING_CAST_PRIVATE_CHROME_NETWORKING_CAST_PRIVATE_DELEGATE_H_
#define CHROME_BROWSER_EXTENSIONS_API_NETWORKING_CAST_PRIVATE_CHROME_NETWORKING_CAST_PRIVATE_DELEGATE_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "extensions/browser/api/networking_private/networking_cast_private_delegate.h"

namespace extensions {

// Chrome implementation of extensions::NetworkingCastPrivateDelegate.
class ChromeNetworkingCastPrivateDelegate
    : public NetworkingCastPrivateDelegate {
 public:
  using FactoryCallback =
      base::Callback<std::unique_ptr<ChromeNetworkingCastPrivateDelegate>()>;
  static void SetFactoryCallbackForTest(FactoryCallback* factory_callback);

  static std::unique_ptr<ChromeNetworkingCastPrivateDelegate> Create();

  ~ChromeNetworkingCastPrivateDelegate() override;

  // NetworkingCastPrivateDelegate overrides:
  void VerifyDestination(std::unique_ptr<Credentials> credentials,
                         const VerifiedCallback& success_callback,
                         const FailureCallback& failure_callback) override;
  void VerifyAndEncryptCredentials(
      const std::string& guid,
      std::unique_ptr<Credentials> credentials,
      const DataCallback& success_callback,
      const FailureCallback& failure_callback) override;
  void VerifyAndEncryptData(const std::string& data,
                            std::unique_ptr<Credentials> credentials,
                            const DataCallback& success_callback,
                            const FailureCallback& failure_callback) override;

 protected:
  ChromeNetworkingCastPrivateDelegate();

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeNetworkingCastPrivateDelegate);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_NETWORKING_CAST_PRIVATE_CHROME_NETWORKING_CAST_PRIVATE_DELEGATE_H_
