// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/extension_process_policy.h"

#include "base/strings/string_util.h"
#include "chrome/common/extensions/extension_constants.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/manifest_handlers/app_isolation_info.h"
#include "extensions/common/switches.h"

namespace extensions {

const extensions::Extension* GetNonBookmarkAppExtension(
    const ExtensionSet& extensions,
    const GURL& url) {
  // Exclude bookmark apps, which do not use the app process model.
  const extensions::Extension* extension =
      extensions.GetExtensionOrAppByURL(url);
  if (extension && extension->from_bookmark())
    extension = NULL;
  return extension;
}

bool CrossesExtensionProcessBoundary(const ExtensionSet& extensions,
                                     const GURL& old_url,
                                     const GURL& new_url,
                                     bool should_consider_workaround) {
  const extensions::Extension* old_url_extension =
      GetNonBookmarkAppExtension(extensions, old_url);
  const extensions::Extension* new_url_extension =
      GetNonBookmarkAppExtension(extensions, new_url);

  // TODO(creis): Temporary workaround for crbug.com/59285: Do not swap process
  // to navigate from a hosted app to a normal page or another hosted app
  // (unless either is the web store).  This is because some OAuth providers
  // use non-app popups that communicate with non-app iframes inside the app
  // (e.g., Facebook).  This would require out-of-process iframes to support.
  // See http://crbug.com/99379.
  // Note that we skip this exception for isolated apps, which require strict
  // process separation from non-app pages.
  if (should_consider_workaround) {
    bool old_url_is_hosted_app =
        old_url_extension && !old_url_extension->web_extent().is_empty() &&
        !AppIsolationInfo::HasIsolatedStorage(old_url_extension);
    bool new_url_is_normal_or_hosted =
        !new_url_extension ||
        (!new_url_extension->web_extent().is_empty() &&
         !AppIsolationInfo::HasIsolatedStorage(new_url_extension));
    bool either_is_web_store =
        (old_url_extension &&
         old_url_extension->id() == extensions::kWebStoreAppId) ||
        (new_url_extension &&
         new_url_extension->id() == extensions::kWebStoreAppId);
    if (old_url_is_hosted_app && new_url_is_normal_or_hosted &&
        !either_is_web_store)
      return false;
  }

  // If there are no extensions associated with either url, we check if the new
  // url points to an extension origin. If it does, fork - extension
  // installation should not be a factor.
  if (!old_url_extension && !new_url_extension) {
    // Hypothetically, we could also do an origin check here to make sure that
    // the two urls point two different extensions, but it's not really
    // necesary since we know there wasn't an associated extension with the old
    // url.
    return new_url.SchemeIs(kExtensionScheme);
  }

  return old_url_extension != new_url_extension;
}

}  // namespace extensions
