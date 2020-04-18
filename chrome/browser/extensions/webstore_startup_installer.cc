// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/webstore_startup_installer.h"

#include <memory>

namespace extensions {

WebstoreStartupInstaller::WebstoreStartupInstaller(
    const std::string& webstore_item_id,
    Profile* profile,
    bool show_prompt,
    const Callback& callback)
    : WebstoreInstallWithPrompt(webstore_item_id, profile, callback),
      show_prompt_(show_prompt) {
  set_install_source(WebstoreInstaller::INSTALL_SOURCE_INLINE);
  set_show_post_install_ui(false);
}

WebstoreStartupInstaller::~WebstoreStartupInstaller() {}

std::unique_ptr<ExtensionInstallPrompt::Prompt>
WebstoreStartupInstaller::CreateInstallPrompt() const {
  if (show_prompt_) {
    return std::make_unique<ExtensionInstallPrompt::Prompt>(
        ExtensionInstallPrompt::INSTALL_PROMPT);
  }
  return NULL;
}

}  // namespace extensions
