// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/extension_uninstaller.h"

#include <string>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/uninstall_reason.h"
#include "extensions/common/extension.h"

ExtensionUninstaller::ExtensionUninstaller(
    Profile* profile,
    const std::string& extension_id,
    AppListControllerDelegate* controller)
    : profile_(profile),
      app_id_(extension_id),
      controller_(controller) {
}

ExtensionUninstaller::~ExtensionUninstaller() {
}

void ExtensionUninstaller::Run() {
  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(profile_)->GetInstalledExtension(
          app_id_);
  if (!extension) {
    CleanUp();
    return;
  }
  controller_->OnShowChildDialog();
  dialog_.reset(
      extensions::ExtensionUninstallDialog::Create(profile_, nullptr, this));
  dialog_->ConfirmUninstall(extension,
                            extensions::UNINSTALL_REASON_USER_INITIATED,
                            extensions::UNINSTALL_SOURCE_APP_LIST);
}

void ExtensionUninstaller::OnExtensionUninstallDialogClosed(
    bool did_start_uninstall,
    const base::string16& error) {
  controller_->OnCloseChildDialog();
  CleanUp();
}

void ExtensionUninstaller::CleanUp() {
  delete this;
}
