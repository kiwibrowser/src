// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_SMB_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_SMB_HANDLER_H_

#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/smb_client/smb_service.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class Profile;

namespace chromeos {
namespace settings {

using smb_client::SmbMountResult;

class SmbHandler : public ::settings::SettingsPageUIHandler {
 public:
  explicit SmbHandler(Profile* profile);
  ~SmbHandler() override;

  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

 private:
  // WebUI call to mount an Smb Filesystem.
  void HandleSmbMount(const base::ListValue* args);

  // Callback handler for SmbMount.
  void HandleSmbMountResponse(SmbMountResult result);

  Profile* const profile_;
  base::WeakPtrFactory<SmbHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SmbHandler);
};

}  // namespace settings
}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_SMB_HANDLER_H_
