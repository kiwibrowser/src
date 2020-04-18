// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_BROWSER_EXTENSION_WINDOW_CONTROLLER_H_
#define CHROME_BROWSER_EXTENSIONS_BROWSER_EXTENSION_WINDOW_CONTROLLER_H_

#include "base/macros.h"
#include "chrome/browser/extensions/window_controller.h"

class Browser;

namespace extensions {
class Extension;

class BrowserExtensionWindowController : public WindowController {
 public:
  explicit BrowserExtensionWindowController(Browser* browser);
  ~BrowserExtensionWindowController() override;

  // WindowController implementation.
  int GetWindowId() const override;
  std::string GetWindowTypeText() const override;
  bool CanClose(Reason* reason) const override;
  void SetFullscreenMode(bool is_fullscreen,
                         const GURL& extension_url) const override;
  Browser* GetBrowser() const override;
  bool IsVisibleToTabsAPIForExtension(
      const Extension* extension,
      bool allow_dev_tools_windows) const override;

 private:
  Browser* const browser_;

  DISALLOW_COPY_AND_ASSIGN(BrowserExtensionWindowController);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_BROWSER_EXTENSION_WINDOW_CONTROLLER_H_
