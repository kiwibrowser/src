// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/extensions_client.h"

#include "base/logging.h"
#include "extensions/common/extension_icon_set.h"
#include "extensions/common/manifest_handlers/icons_handler.h"

namespace extensions {

namespace {

ExtensionsClient* g_client = NULL;

}  // namespace

ExtensionsClient* ExtensionsClient::Get() {
  DCHECK(g_client);
  return g_client;
}

std::set<base::FilePath> ExtensionsClient::GetBrowserImagePaths(
    const Extension* extension) {
  std::set<base::FilePath> paths;
  extensions::IconsInfo::GetIcons(extension).GetPaths(&paths);
  return paths;
}

bool ExtensionsClient::ExtensionAPIEnabledInExtensionServiceWorkers() const {
  return false;
}

std::string ExtensionsClient::GetUserAgent() const {
  return std::string();
}

void ExtensionsClient::Set(ExtensionsClient* client) {
  // This can happen in unit tests, where the utility thread runs in-process.
  if (g_client)
    return;
  g_client = client;
  g_client->Initialize();
}

}  // namespace extensions
