// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/smb_provider.h"

#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_interface.h"
#include "chrome/browser/chromeos/file_system_provider/service.h"
#include "chrome/browser/chromeos/smb_client/smb_file_system.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/settings_window_manager_chromeos.h"
#include "chrome/common/webui_url_constants.h"
#include "url/gurl.h"

namespace chromeos {
namespace smb_client {

SmbProvider::SmbProvider(UnmountCallback unmount_callback)
    : provider_id_(ProviderId::CreateFromNativeId("smb")),
      capabilities_(false /* configurable */,
                    false /* watchable */,
                    true /* multiple_mounts */,
                    extensions::SOURCE_NETWORK),
      // TODO(baileyberro): Localize this string, so it shows correctly in all
      // languages. See l10n_util::GetStringUTF8.
      name_("SMB Shares"),
      unmount_callback_(std::move(unmount_callback)) {
  // TODO(baileyberro): Fill out with proper icons.
  icon_set_.SetIcon(IconSet::IconSize::SIZE_16x16,
                    GURL("chrome://resources/images/apps/button.png"));
  icon_set_.SetIcon(IconSet::IconSize::SIZE_32x32,
                    GURL("chrome://resources/images/2x/apps/button.png"));
}

SmbProvider::~SmbProvider() = default;

std::unique_ptr<ProvidedFileSystemInterface>
SmbProvider::CreateProvidedFileSystem(
    Profile* profile,
    const ProvidedFileSystemInfo& file_system_info) {
  DCHECK(profile);
  return std::make_unique<SmbFileSystem>(file_system_info, unmount_callback_);
}

const Capabilities& SmbProvider::GetCapabilities() const {
  return capabilities_;
}

const ProviderId& SmbProvider::GetId() const {
  return provider_id_;
}

const std::string& SmbProvider::GetName() const {
  return name_;
}

const IconSet& SmbProvider::GetIconSet() const {
  return icon_set_;
}

bool SmbProvider::RequestMount(Profile* profile) {
  auto* settings_manager = chrome::SettingsWindowManager::GetInstance();
  settings_manager->ShowChromePageForProfile(
      profile, chrome::GetSettingsUrl(chrome::kSmbSharesPage));

  return true;
}

}  // namespace smb_client
}  // namespace chromeos
