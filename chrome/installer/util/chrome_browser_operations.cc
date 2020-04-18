// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/chrome_browser_operations.h"

#include "base/logging.h"
#include "chrome/install_static/install_util.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"
#include "chrome/installer/util/util_constants.h"

namespace installer {

void ChromeBrowserOperations::AddKeyFiles(
    std::vector<base::FilePath>* key_files) const {
  DCHECK(key_files);
  key_files->push_back(base::FilePath(kChromeDll));
}

// Modifies a ShortcutProperties object by adding default values to
// uninitialized members. Tries to assign:
// - target: |chrome_exe|.
// - icon: from |chrome_exe|.
// - icon_index: |dist|'s icon index
// - app_id: the browser model id for the current install.
// - description: |dist|'s description.
void ChromeBrowserOperations::AddDefaultShortcutProperties(
      BrowserDistribution* dist,
      const base::FilePath& target_exe,
      ShellUtil::ShortcutProperties* properties) const {
  if (!properties->has_target())
    properties->set_target(target_exe);

  if (!properties->has_icon()) {
    DCHECK_EQ(BrowserDistribution::GetDistribution(), dist);
    properties->set_icon(target_exe, install_static::GetIconResourceIndex());
  }

  if (!properties->has_app_id()) {
    DCHECK_EQ(BrowserDistribution::GetDistribution(), dist);
    properties->set_app_id(
        ShellUtil::GetBrowserModelId(InstallUtil::IsPerUserInstall()));
  }

  if (!properties->has_description())
    properties->set_description(dist->GetAppDescription());
}

}  // namespace installer
