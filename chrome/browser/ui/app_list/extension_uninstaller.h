// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_EXTENSION_UNINSTALLER_H_
#define CHROME_BROWSER_UI_APP_LIST_EXTENSION_UNINSTALLER_H_

#include "base/macros.h"
#include "chrome/browser/extensions/extension_uninstall_dialog.h"

class AppListControllerDelegate;
class Profile;

// ExtensionUninstaller runs the extension uninstall flow. It shows the
// extension uninstall dialog and wait for user to confirm or cancel the
// uninstall.
class ExtensionUninstaller
    : public extensions::ExtensionUninstallDialog::Delegate {
 public:
  ExtensionUninstaller(Profile* profile,
                       const std::string& extension_id,
                       AppListControllerDelegate* controller);
  ~ExtensionUninstaller() override;

  void Run();

 private:
  // Overridden from ExtensionUninstallDialog::Delegate:
  void OnExtensionUninstallDialogClosed(bool did_start_uninstall,
                                        const base::string16& error) override;

  void CleanUp();

  Profile* profile_;
  std::string app_id_;
  AppListControllerDelegate* controller_;
  std::unique_ptr<extensions::ExtensionUninstallDialog> dialog_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionUninstaller);
};


#endif  // CHROME_BROWSER_UI_APP_LIST_EXTENSION_UNINSTALLER_H_
