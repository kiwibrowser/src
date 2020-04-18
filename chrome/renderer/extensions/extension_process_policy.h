// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_EXTENSIONS_EXTENSION_PROCESS_POLICY_H_
#define CHROME_RENDERER_EXTENSIONS_EXTENSION_PROCESS_POLICY_H_

class GURL;

namespace extensions {

class Extension;
class ExtensionSet;

// Returns the extension for the given URL.  Excludes extension objects for
// bookmark apps, which do not use the app process model.
const Extension* GetNonBookmarkAppExtension(const ExtensionSet& extensions,
                                            const GURL& url);

// Check if navigating a toplevel page from |old_url| to |new_url| would cross
// an extension process boundary (e.g. navigating from a web URL into an
// extension URL).
// We temporarily consider a workaround where we will keep non-app URLs in
// an app process, but only if |should_consider_workaround| is true.  See
// http://crbug.com/59285.
bool CrossesExtensionProcessBoundary(const ExtensionSet& extensions,
                                     const GURL& old_url,
                                     const GURL& new_url,
                                     bool should_consider_workaround);

}  // namespace extensions

#endif  // CHROME_RENDERER_EXTENSIONS_EXTENSION_PROCESS_POLICY_H_
