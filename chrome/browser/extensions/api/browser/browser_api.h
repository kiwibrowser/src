// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_BROWSER_BROWSER_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_BROWSER_BROWSER_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class BrowserOpenTabFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("browser.openTab", BROWSER_OPENTAB)

 protected:
  ~BrowserOpenTabFunction() override;

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_BROWSER_BROWSER_API_H_
