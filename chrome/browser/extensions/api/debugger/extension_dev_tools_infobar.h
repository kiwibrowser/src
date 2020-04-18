// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_DEBUGGER_EXTENSION_DEV_TOOLS_INFOBAR_H_
#define CHROME_BROWSER_EXTENSIONS_API_DEBUGGER_EXTENSION_DEV_TOOLS_INFOBAR_H_

#include <map>
#include <string>

#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"

class GlobalConfirmInfoBar;

namespace extensions {
class ExtensionDevToolsClientHost;

// An infobar used to globally warn users that an extension is debugging the
// browser (which has security consequences).
class ExtensionDevToolsInfoBar {
 public:
  static ExtensionDevToolsInfoBar* Create(
      const std::string& extension_id,
      const std::string& extension_name,
      ExtensionDevToolsClientHost* client_host,
      const base::Closure& dismissed_callback);
  void Remove(ExtensionDevToolsClientHost* client_host);

 private:
  ExtensionDevToolsInfoBar(const std::string& extension_id,
                           const std::string& extension_name);
  ~ExtensionDevToolsInfoBar();
  void InfoBarDismissed();

  std::string extension_id_;
  std::map<ExtensionDevToolsClientHost*, base::Closure> callbacks_;
  base::WeakPtr<GlobalConfirmInfoBar> infobar_;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_DEBUGGER_EXTENSION_DEV_TOOLS_INFOBAR_H_
