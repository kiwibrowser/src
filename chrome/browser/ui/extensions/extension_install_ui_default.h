// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_EXTENSIONS_EXTENSION_INSTALL_UI_DEFAULT_H_
#define CHROME_BROWSER_UI_EXTENSIONS_EXTENSION_INSTALL_UI_DEFAULT_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "extensions/browser/install/extension_install_ui.h"

namespace content {
class BrowserContext;
}

class Profile;

class ExtensionInstallUIDefault : public extensions::ExtensionInstallUI {
 public:
  explicit ExtensionInstallUIDefault(content::BrowserContext* context);
  ~ExtensionInstallUIDefault() override;

  // ExtensionInstallUI:
  void OnInstallSuccess(const extensions::Extension* extension,
                        const SkBitmap* icon) override;
  void OnInstallFailure(const extensions::CrxInstallError& error) override;
  void SetUseAppInstalledBubble(bool use_bubble) override;
  void OpenAppInstalledUI(const std::string& app_id) override;
  void SetSkipPostInstallUI(bool skip_ui) override;
  gfx::NativeWindow GetDefaultInstallDialogParent() override;

 private:
  Profile* profile_;

  // Whether or not to show the default UI after completing the installation.
  bool skip_post_install_ui_;

  // Whether to show an installed bubble on app install, or use the default
  // action of opening a new tab page.
  bool use_app_installed_bubble_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionInstallUIDefault);
};

#endif  // CHROME_BROWSER_UI_EXTENSIONS_EXTENSION_INSTALL_UI_DEFAULT_H_
