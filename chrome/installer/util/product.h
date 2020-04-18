// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_UTIL_PRODUCT_H_
#define CHROME_INSTALLER_UTIL_PRODUCT_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "chrome/installer/util/shell_util.h"

class BrowserDistribution;

namespace base {
class CommandLine;
}

namespace installer {

class Product;
class ProductOperations;

// Represents an installation of a specific product which has a one-to-one
// relation to a BrowserDistribution.  A product has registry settings, related
// installation/uninstallation actions and exactly one Package that represents
// the files on disk.  The Package may be shared with other Product instances,
// so only the last Product to be uninstalled should remove the package.
// Right now there are no classes that derive from Product, but in
// the future, as we move away from global functions and towards a data driven
// installation, each distribution could derive from this class and provide
// distribution specific functionality.
class Product {
 public:
  explicit Product(BrowserDistribution* distribution);

  ~Product();

  BrowserDistribution* distribution() const {
    return distribution_;
  }

  // Launches Chrome without waiting for it to exit.
  bool LaunchChrome(const base::FilePath& application_path) const;

  // Launches Chrome with given command line, waits for Chrome indefinitely
  // (until it terminates), and gets the process exit code if available.
  // The function returns true as long as Chrome is successfully launched.
  // The status of Chrome at the return of the function is given by exit_code.
  // NOTE: The 'options' CommandLine object should only contain parameters.
  // The program part will be ignored.
  bool LaunchChromeAndWait(const base::FilePath& application_path,
                           const base::CommandLine& options,
                           int32_t* exit_code) const;

  // Sets the boolean MSI marker for this installation if set is true or clears
  // it otherwise. The MSI marker is stored in the registry under the
  // ClientState key.
  bool SetMsiMarker(bool system_install, bool set) const;

  // See ProductOperations::AddKeyFiles.
  void AddKeyFiles(std::vector<base::FilePath>* key_files) const;

  // See ProductOperations::AddDefaultShortcutProperties.
  void AddDefaultShortcutProperties(
      const base::FilePath& target_exe,
      ShellUtil::ShortcutProperties* properties) const;

 protected:
  enum CacheStateFlags {
    MSI_STATE = 0x01
  };

  BrowserDistribution* const distribution_;
  const std::unique_ptr<ProductOperations> operations_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Product);
};

}  // namespace installer

#endif  // CHROME_INSTALLER_UTIL_PRODUCT_H_
