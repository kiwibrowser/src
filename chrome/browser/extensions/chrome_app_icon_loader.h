// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_CHROME_APP_ICON_LOADER_H_
#define CHROME_BROWSER_EXTENSIONS_CHROME_APP_ICON_LOADER_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/extensions/chrome_app_icon_delegate.h"
#include "chrome/browser/ui/app_icon_loader.h"

class Profile;

namespace extensions {

// Implementation of AppIconLoader that uses ChromeAppIcon to load and update
// Chrome app images.
class ChromeAppIconLoader : public AppIconLoader, public ChromeAppIconDelegate {
 public:
  ChromeAppIconLoader(Profile* profile,
                      int icon_size_in_dips,
                      AppIconLoaderDelegate* delegate);
  ~ChromeAppIconLoader() override;

  // AppIconLoader overrides:
  bool CanLoadImageForApp(const std::string& id) override;
  void FetchImage(const std::string& id) override;
  void ClearImage(const std::string& id) override;
  void UpdateImage(const std::string& id) override;

 private:
  using ExtensionIDToChromeAppIconMap =
      std::map<std::string, std::unique_ptr<ChromeAppIcon>>;

  // ChromeAppIconDelegate:
  void OnIconUpdated(ChromeAppIcon* icon) override;

  // Maps from extension id to ChromeAppIcon.
  ExtensionIDToChromeAppIconMap map_;

  DISALLOW_COPY_AND_ASSIGN(ChromeAppIconLoader);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_CHROME_APP_ICON_LOADER_H_
