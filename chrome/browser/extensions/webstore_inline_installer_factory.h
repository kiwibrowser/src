// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_WEBSTORE_INLINE_INSTALLER_FACTORY_H_
#define CHROME_BROWSER_EXTENSIONS_WEBSTORE_INLINE_INSTALLER_FACTORY_H_

#include <memory>
#include <string>

#include "chrome/browser/extensions/extension_install_prompt.h"
#include "chrome/browser/extensions/webstore_standalone_installer.h"

namespace content {
class WebContents;
}

class GURL;

namespace extensions {

class WebstoreInlineInstaller;

class WebstoreInlineInstallerFactory {
 public:
  virtual ~WebstoreInlineInstallerFactory() {}

  // Create a new WebstoreInlineInstallerInstance to be owned by the caller.
  virtual WebstoreInlineInstaller* CreateInstaller(
      content::WebContents* contents,
      content::RenderFrameHost* host,
      const std::string& webstore_item_id,
      const GURL& requestor_url,
      const WebstoreStandaloneInstaller::Callback& callback);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_WEBSTORE_INLINE_INSTALLER_FACTORY_H_
