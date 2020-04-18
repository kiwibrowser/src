// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_UTIL_CHROME_BROWSER_OPERATIONS_H_
#define CHROME_INSTALLER_UTIL_CHROME_BROWSER_OPERATIONS_H_

#include "base/macros.h"
#include "chrome/installer/util/product_operations.h"

namespace installer {

// Operations specific to Chrome; see ProductOperations for general info.
class ChromeBrowserOperations : public ProductOperations {
 public:
  ChromeBrowserOperations() {}

  void AddKeyFiles(std::vector<base::FilePath>* key_files) const override;

  void AddDefaultShortcutProperties(
      BrowserDistribution* dist,
      const base::FilePath& target_exe,
      ShellUtil::ShortcutProperties* properties) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserOperations);
};

}  // namespace installer

#endif  // CHROME_INSTALLER_UTIL_CHROME_BROWSER_OPERATIONS_H_
