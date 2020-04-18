// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_INLINE_INSTALL_PRIVATE_INLINE_INSTALL_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_INLINE_INSTALL_PRIVATE_INLINE_INSTALL_PRIVATE_API_H_

#include <memory>

#include "chrome/common/extensions/webstore_install_result.h"
#include "extensions/browser/extension_function.h"

namespace extensions {

class InlineInstallPrivateInstallFunction
    : public UIThreadExtensionFunction {
 public:
  InlineInstallPrivateInstallFunction();

 protected:
  ~InlineInstallPrivateInstallFunction() override;

  ResponseAction Run() override;

 private:
  void InstallerCallback(bool success,
                         const std::string& error,
                         webstore_install::Result result);

  // Helper to create a response to be returned either synchronously or in
  // InstallerCallback.
  ResponseValue CreateResponse(const std::string& error,
                               webstore_install::Result result);

  DECLARE_EXTENSION_FUNCTION("inlineInstallPrivate.install",
                             INLINE_INSTALL_PRIVATE_INSTALL);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_INLINE_INSTALL_PRIVATE_INLINE_INSTALL_PRIVATE_API_H_
