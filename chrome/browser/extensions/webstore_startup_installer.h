// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_WEBSTORE_STARTUP_INSTALLER_H_
#define CHROME_BROWSER_EXTENSIONS_WEBSTORE_STARTUP_INSTALLER_H_

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"

namespace extensions {

// Manages inline installs requested to be performed at startup, e.g. via a
// command line option: downloads and parses metadata from the webstore,
// optionally shows an install UI, starts the download once the user
// confirms.
//
// Clients will be notified of success or failure via the |callback| argument
// passed into the constructor.
class WebstoreStartupInstaller : public WebstoreInstallWithPrompt {
 public:
  WebstoreStartupInstaller(const std::string& webstore_item_id,
                           Profile* profile,
                           bool show_prompt,
                           const Callback& callback);

 protected:
  friend class base::RefCountedThreadSafe<WebstoreStartupInstaller>;
  FRIEND_TEST_ALL_PREFIXES(WebstoreStartupInstallerTest, DomainVerification);

  ~WebstoreStartupInstaller() override;

  // Implementations of WebstoreStandaloneInstaller Template Method's hooks.
  std::unique_ptr<ExtensionInstallPrompt::Prompt> CreateInstallPrompt()
      const override;

 private:
  bool show_prompt_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(WebstoreStartupInstaller);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_WEBSTORE_STARTUP_INSTALLER_H_
