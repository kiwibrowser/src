// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/webstore_inline_installer_factory.h"

#include <memory>

#include "chrome/browser/extensions/webstore_inline_installer.h"
#include "content/public/browser/web_contents.h"

namespace extensions {

WebstoreInlineInstaller* WebstoreInlineInstallerFactory::CreateInstaller(
    content::WebContents* contents,
    content::RenderFrameHost* host,
    const std::string& webstore_item_id,
    const GURL& requestor_url,
    const WebstoreStandaloneInstaller::Callback& callback) {
  return new WebstoreInlineInstaller(contents, host, webstore_item_id,
                                     requestor_url, callback);
}

}  // namespace extensions
