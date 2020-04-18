// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_WEBSTORE_WEBSTORE_INSTALLER_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_WEBSTORE_WEBSTORE_INSTALLER_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"

class Profile;

namespace app_list {

// WebstoreInstaller handles install for web store search results.
class WebstoreInstaller : public extensions::WebstoreInstallWithPrompt {
 public:
  typedef WebstoreStandaloneInstaller::Callback Callback;

  WebstoreInstaller(const std::string& webstore_item_id,
                    Profile* profile,
                    const Callback& callback);

 private:
  friend class base::RefCountedThreadSafe<WebstoreInstaller>;
  ~WebstoreInstaller() override;

  DISALLOW_COPY_AND_ASSIGN(WebstoreInstaller);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_WEBSTORE_WEBSTORE_INSTALLER_H_
