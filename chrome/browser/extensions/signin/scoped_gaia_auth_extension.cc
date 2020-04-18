// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/signin/scoped_gaia_auth_extension.h"

#include "chrome/browser/extensions/signin/gaia_auth_extension_loader.h"
#include "content/public/browser/browser_context.h"

ScopedGaiaAuthExtension::ScopedGaiaAuthExtension(
    content::BrowserContext* context)
    : browser_context_(context) {
  extensions::GaiaAuthExtensionLoader* loader =
      extensions::GaiaAuthExtensionLoader::Get(browser_context_);
  if (loader)
    loader->LoadIfNeeded();
}

ScopedGaiaAuthExtension::~ScopedGaiaAuthExtension() {
  extensions::GaiaAuthExtensionLoader* loader =
      extensions::GaiaAuthExtensionLoader::Get(browser_context_);
  if (loader)
    loader->UnloadIfNeededAsync();
}
