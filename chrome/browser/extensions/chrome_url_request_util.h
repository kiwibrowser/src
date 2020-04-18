// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_CHROME_URL_REQUEST_UTIL_H_
#define CHROME_BROWSER_EXTENSIONS_CHROME_URL_REQUEST_UTIL_H_

#include <string>

#include "content/public/common/resource_type.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "ui/base/page_transition_types.h"

class GURL;

namespace base {
class FilePath;
}

namespace net {
class NetworkDelegate;
class URLRequest;
class URLRequestJob;
}

namespace extensions {
class Extension;
class ExtensionSet;
class ProcessMap;

// Utilities related to URLRequest jobs for extension resources. See
// chrome/browser/extensions/extension_protocols_unittest.cc for related tests.
namespace chrome_url_request_util {

// Sets allowed=true to allow a chrome-extension:// resource request coming from
// renderer A to access a resource in an extension running in renderer B.
// Returns false when it couldn't determine if the resource is allowed or not
bool AllowCrossRendererResourceLoad(const GURL& url,
                                    content::ResourceType resource_type,
                                    ui::PageTransition page_transition,
                                    int child_id,
                                    bool is_incognito,
                                    const Extension* extension,
                                    const ExtensionSet& extensions,
                                    const ProcessMap& process_map,
                                    bool* allowed);

// Creates a URLRequestJob for loading component extension resources out of
// a Chrome resource bundle. Returns NULL if the requested resource is not a
// component extension resource.
net::URLRequestJob* MaybeCreateURLRequestResourceBundleJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const base::FilePath& directory_path,
    const std::string& content_security_policy,
    bool send_cors_header);

// Return the |request|'s resource path relative to the Chromium resources path
// (chrome::DIR_RESOURCES) *if* the request refers to a resource within the
// Chrome resource bundle. If not then the returned file path will be empty.
base::FilePath GetBundleResourcePath(
    const network::ResourceRequest& request,
    const base::FilePath& extension_resources_path,
    int* resource_id);

// Creates and starts a URLLoader for loading component extension resources out
// of a Chrome resource bundle. This should only be called if
// GetBundleResourcePath returns a valid path.
void LoadResourceFromResourceBundle(
    const network::ResourceRequest& request,
    network::mojom::URLLoaderRequest loader,
    const base::FilePath& resource_relative_path,
    int resource_id,
    const std::string& content_security_policy,
    network::mojom::URLLoaderClientPtr client,
    bool send_cors_header);

}  // namespace chrome_url_request_util
}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_CHROME_URL_REQUEST_UTIL_H_
