// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_UPDATER_CHROMEOS_EXTENSION_CACHE_DELEGATE_H_
#define CHROME_BROWSER_EXTENSIONS_UPDATER_CHROMEOS_EXTENSION_CACHE_DELEGATE_H_

#include <stddef.h>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "chrome/browser/extensions/updater/extension_cache_delegate.h"

namespace extensions {

// Chrome OS-specific implementation, which has a pre-defined extension cache
// path and a policy-configurable maximum cache size.
class ChromeOSExtensionCacheDelegate : public ExtensionCacheDelegate {
 public:
  ChromeOSExtensionCacheDelegate();
  explicit ChromeOSExtensionCacheDelegate(const base::FilePath& cache_dir);

  const base::FilePath& GetCacheDir() const override;
  size_t GetMaximumCacheSize() const override;

 private:
  base::FilePath cache_dir_;

  DISALLOW_COPY_AND_ASSIGN(ChromeOSExtensionCacheDelegate);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_UPDATER_CHROMEOS_EXTENSION_CACHE_DELEGATE_H_
