// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines dummy implementation of several functions from the
// BrowserDistribution class for Google Chrome. These functions allow 64-bit
// Windows Chrome binary to build successfully. Since this binary is only used
// for Native Client support, most of the install/uninstall functionality is not
// necessary there.

#include "chrome/installer/util/google_chrome_distribution.h"

#include <windows.h>

#include <utility>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/values.h"
#include "chrome/installer/util/app_registration_data.h"
#include "chrome/installer/util/non_updating_app_registration_data.h"

GoogleChromeDistribution::GoogleChromeDistribution()
    : BrowserDistribution(
          CHROME_BROWSER,
          std::unique_ptr<AppRegistrationData>(
              new NonUpdatingAppRegistrationData(base::string16()))) {}

GoogleChromeDistribution::GoogleChromeDistribution(
    std::unique_ptr<AppRegistrationData> app_reg_data)
    : BrowserDistribution(CHROME_BROWSER, std::move(app_reg_data)) {}

void GoogleChromeDistribution::DoPostUninstallOperations(
    const Version& version,
    const base::FilePath& local_data_path,
    const base::string16& distribution_data) {
}

base::string16 GoogleChromeDistribution::GetShortcutName() {
  return base::string16();
}

base::string16 GoogleChromeDistribution::GetPublisherName() {
  return base::string16();
}

base::string16 GoogleChromeDistribution::GetAppDescription() {
  return base::string16();
}

std::string GoogleChromeDistribution::GetSafeBrowsingName() {
  return std::string();
}

base::string16 GoogleChromeDistribution::GetDistributionData(HKEY root_key) {
  return base::string16();
}

void GoogleChromeDistribution::UpdateInstallStatus(bool system_install,
    installer::ArchiveType archive_type,
    installer::InstallStatus install_status) {
}
