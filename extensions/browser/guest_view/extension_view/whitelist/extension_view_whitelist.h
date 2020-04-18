// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_GUEST_VIEW_EXTENSION_VIEW_WHITELIST_EXTENSION_VIEW_WHITELIST_H_
#define EXTENSIONS_BROWSER_GUEST_VIEW_EXTENSION_VIEW_WHITELIST_EXTENSION_VIEW_WHITELIST_H_

#include <string>

namespace extensions {

// Checks whether |extension_id| is whitelisted to be used by ExtensionView.
bool IsExtensionIdWhitelisted(const std::string& extension_id);

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_GUEST_VIEW_EXTENSION_VIEW_WHITELIST_EXTENSION_VIEW_WHITELIST_H_
