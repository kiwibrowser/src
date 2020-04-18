// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_EXTERNAL_CACHE_DELEGATE_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_EXTERNAL_CACHE_DELEGATE_H_

#include <string>

namespace base {
class DictionaryValue;
}

namespace chromeos {

class ExternalCacheDelegate {
 public:
  virtual ~ExternalCacheDelegate() = default;

  // Caller owns |prefs|.
  virtual void OnExtensionListsUpdated(const base::DictionaryValue* prefs) = 0;

  // Called after extension with |id| is loaded in cache.
  virtual void OnExtensionLoadedInCache(const std::string& id) = 0;

  // Called when extension with |id| fails to load due to a download error.
  virtual void OnExtensionDownloadFailed(const std::string& id) = 0;

  // Cache needs to provide already installed extensions otherwise they
  // will be removed. Cache calls this function to get version of installed
  // extension or empty string if not installed.
  virtual std::string GetInstalledExtensionVersion(const std::string& id) = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_EXTERNAL_CACHE_DELEGATE_H_
