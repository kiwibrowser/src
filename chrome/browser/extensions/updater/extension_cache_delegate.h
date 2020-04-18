// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_UPDATER_EXTENSION_CACHE_DELEGATE_H_
#define CHROME_BROWSER_EXTENSIONS_UPDATER_EXTENSION_CACHE_DELEGATE_H_

#include <stddef.h>

#include "base/macros.h"
#include "base/time/time.h"

namespace base {

class FilePath;

}  // namespace base

namespace extensions {

// This class is used to pass ExtensionCacheImpl's initialization parameters.
// Every function of this class is called only once during ExtensionCacheImpl
// construction.
class ExtensionCacheDelegate {
 public:
  virtual ~ExtensionCacheDelegate();

  virtual const base::FilePath& GetCacheDir() const = 0;

  virtual size_t GetMinimumCacheSize() const;
  virtual size_t GetMaximumCacheSize() const;
  virtual base::TimeDelta GetMaximumCacheAge() const;

 private:
  DISALLOW_ASSIGN(ExtensionCacheDelegate);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_UPDATER_EXTENSION_CACHE_DELEGATE_H_
