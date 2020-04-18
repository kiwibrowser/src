// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_FIRST_RUN_PRIVATE_API_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_FIRST_RUN_PRIVATE_API_H_

#include "base/compiler_specific.h"
#include "chrome/common/extensions/api/first_run_private.h"
#include "extensions/browser/extension_function.h"

class FirstRunPrivateGetLocalizedStringsFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("firstRunPrivate.getLocalizedStrings",
                             FIRSTRUNPRIVATE_GETLOCALIZEDSTRINGS)

 protected:
  ~FirstRunPrivateGetLocalizedStringsFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class FirstRunPrivateLaunchTutorialFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("firstRunPrivate.launchTutorial",
                             FIRSTRUNPRIVATE_LAUNCHTUTORIAL)

 protected:
  ~FirstRunPrivateLaunchTutorialFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_FIRST_RUN_PRIVATE_API_H_
