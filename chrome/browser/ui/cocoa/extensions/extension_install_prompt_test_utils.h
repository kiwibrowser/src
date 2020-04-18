// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_INSTALL_PROMPT_TEST_UTILS_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_INSTALL_PROMPT_TEST_UTILS_H_

#include "base/memory/ref_counted.h"
#include "chrome/browser/extensions/extension_install_prompt.h"

namespace chrome {

// Loads the test extension from the given test directory and manifest file.
scoped_refptr<extensions::Extension> LoadInstallPromptExtension(
    const char* extension_dir_name,
    const char* manifest_file);

// Loads the default install_prompt test extension.
scoped_refptr<extensions::Extension> LoadInstallPromptExtension();

// Loads the icon for the install prompt extension.
gfx::Image LoadInstallPromptIcon();

// Builds a prompt using the given extension.
std::unique_ptr<ExtensionInstallPrompt::Prompt> BuildExtensionInstallPrompt(
    extensions::Extension* extension);

std::unique_ptr<ExtensionInstallPrompt::Prompt>
BuildExtensionPostInstallPermissionsPrompt(extensions::Extension* extension);

}  // namespace chrome

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_INSTALL_PROMPT_TEST_UTILS_H_
