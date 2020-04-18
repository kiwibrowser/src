// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file declares a class that contains various method related to branding.

#ifndef CHROME_INSTALLER_UTIL_BROWSER_DISTRIBUTION_H_
#define CHROME_INSTALLER_UTIL_BROWSER_DISTRIBUTION_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "build/build_config.h"
#include "chrome/installer/util/util_constants.h"

#if defined(OS_WIN)
#include <windows.h>  // NOLINT
#endif

class AppRegistrationData;

namespace base {
class FilePath;
class Version;
}

class BrowserDistribution {
 public:
  enum Subfolder {
    SUBFOLDER_CHROME,
    SUBFOLDER_APPS,
  };

  virtual ~BrowserDistribution();

  static BrowserDistribution* GetDistribution();

  // Getter and adaptors for the underlying |app_reg_data_|.
  const AppRegistrationData& GetAppRegistrationData() const;
  base::string16 GetStateKey() const;
  base::string16 GetStateMediumKey() const;
  base::string16 GetVersionKey() const;

  virtual void DoPostUninstallOperations(
      const base::Version& version,
      const base::FilePath& local_data_path,
      const base::string16& distribution_data);

  // Returns the localized display name of this distribution.
  virtual base::string16 GetDisplayName();

  // Returns the localized name of the Chrome shortcut for this distribution.
  virtual base::string16 GetShortcutName();

  // Returns the localized name of the subfolder in the Start Menu identified by
  // |subfolder_type| that this distribution should create shortcuts in. For
  // SUBFOLDER_CHROME this returns GetShortcutName().
  virtual base::string16 GetStartMenuShortcutSubfolder(
      Subfolder subfolder_type);

  virtual base::string16 GetPublisherName();

  virtual base::string16 GetAppDescription();

  virtual base::string16 GetLongAppDescription();

  virtual std::string GetSafeBrowsingName();

#if defined(OS_WIN)
  virtual base::string16 GetDistributionData(HKEY root_key);
#endif

  virtual void UpdateInstallStatus(bool system_install,
      installer::ArchiveType archive_type,
      installer::InstallStatus install_status);

 protected:
  explicit BrowserDistribution(
      std::unique_ptr<AppRegistrationData> app_reg_data);

  template<class DistributionClass>
  static BrowserDistribution* GetOrCreateBrowserDistribution(
      BrowserDistribution** dist);

  std::unique_ptr<AppRegistrationData> app_reg_data_;

 private:
  BrowserDistribution();

  DISALLOW_COPY_AND_ASSIGN(BrowserDistribution);
};

#endif  // CHROME_INSTALLER_UTIL_BROWSER_DISTRIBUTION_H_
