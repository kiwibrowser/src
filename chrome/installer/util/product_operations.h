// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_UTIL_PRODUCT_OPERATIONS_H_
#define CHROME_INSTALLER_UTIL_PRODUCT_OPERATIONS_H_

#include <vector>

#include "base/files/file_path.h"
#include "chrome/installer/util/shell_util.h"

class BrowserDistribution;

namespace installer {

// An interface to product-specific operations that depend on product
// configuration. Implementations are expected to be stateless.
class ProductOperations {
 public:
  virtual ~ProductOperations() {}

  // A key-file is a file such as a DLL on Windows that is expected to be in use
  // when the product is being used. For example "chrome.dll" for Chrome.
  // Before attempting to delete an installation directory during an
  // uninstallation, the uninstaller will check if any one of a potential set of
  // key files is in use and if they are, abort the delete operation. Only if
  // none of the key files are in use, can the folder be deleted. Note that this
  // function does not return a full path to the key file(s), only (a) file
  // name(s).
  virtual void AddKeyFiles(std::vector<base::FilePath>* key_files) const = 0;

  // Modifies a ShellUtil::ShortcutProperties object by assigning default values
  // to unintialized members.
  virtual void AddDefaultShortcutProperties(
      BrowserDistribution* dist,
      const base::FilePath& target_exe,
      ShellUtil::ShortcutProperties* properties) const = 0;

};

}  // namespace installer

#endif  // CHROME_INSTALLER_UTIL_PRODUCT_OPERATIONS_H_
