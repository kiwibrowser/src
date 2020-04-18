// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/chromeos/smb_handler.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/values.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace chromeos {
namespace settings {

SmbHandler::SmbHandler(Profile* profile)
    : profile_(profile), weak_ptr_factory_(this) {}

SmbHandler::~SmbHandler() = default;

void SmbHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "smbMount",
      base::BindRepeating(&SmbHandler::HandleSmbMount, base::Unretained(this)));
}

void SmbHandler::HandleSmbMount(const base::ListValue* args) {
  CHECK_EQ(3U, args->GetSize());
  std::string mountUrl;
  std::string username;
  std::string password;
  CHECK(args->GetString(0, &mountUrl));
  CHECK(args->GetString(1, &username));
  CHECK(args->GetString(2, &password));

  chromeos::smb_client::SmbService* const service =
      chromeos::smb_client::SmbService::Get(profile_);

  chromeos::file_system_provider::MountOptions mo;
  mo.display_name = mountUrl;
  mo.writable = true;

  service->Mount(mo, base::FilePath(mountUrl), username, password,
                 base::BindOnce(&SmbHandler::HandleSmbMountResponse,
                                weak_ptr_factory_.GetWeakPtr()));
}

void SmbHandler::HandleSmbMountResponse(SmbMountResult result) {
  AllowJavascript();
  FireWebUIListener("on-add-smb-share", base::Value(result));
}

}  // namespace settings
}  // namespace chromeos
