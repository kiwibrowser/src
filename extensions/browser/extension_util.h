// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_UTIL_H_
#define EXTENSIONS_BROWSER_EXTENSION_UTIL_H_

#include <string>

#include "url/gurl.h"

namespace content {
class BrowserContext;
class StoragePartition;
}

namespace extensions {
class Extension;

namespace util {

// TODO(benwells): Move functions from
// chrome/browser/extensions/extension_util.h/cc that are only dependent on
// extensions/ here.

// Returns true if the site URL corresponds to an extension or app and has
// isolated storage.
bool SiteHasIsolatedStorage(const GURL& extension_site_url,
                            content::BrowserContext* context);

// Returns true if the extension can be enabled in incognito mode.
bool CanBeIncognitoEnabled(const Extension* extension);

// Returns true if |extension_id| can run in an incognito window.
bool IsIncognitoEnabled(const std::string& extension_id,
                        content::BrowserContext* context);

// Returns the site of the |extension_id|, given the associated |context|.
// Suitable for use with BrowserContext::GetStoragePartitionForSite().
GURL GetSiteForExtensionId(const std::string& extension_id,
                           content::BrowserContext* context);

content::StoragePartition* GetStoragePartitionForExtensionId(
    const std::string& extension_id,
    content::BrowserContext* browser_context);

}  // namespace util
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_UTIL_H_
