// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_TERMINAL_TERMINAL_EXTENSION_HELPER_H_
#define CHROME_BROWSER_EXTENSIONS_API_TERMINAL_TERMINAL_EXTENSION_HELPER_H_

#include <string>

#include "url/gurl.h"

class Profile;

namespace extensions {

class TerminalExtensionHelper {
 public:
  // Returns Hterm extension's entry point for Crosh. If no HTerm extension is
  // installed, returns empty url.
  static GURL GetCroshExtensionURL(Profile* profile);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_TERMINAL_TERMINAL_EXTENSION_HELPER_H_
