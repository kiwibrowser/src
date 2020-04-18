// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_PROTOCOLS_H_
#define EXTENSIONS_BROWSER_EXTENSION_PROTOCOLS_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_request_job_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace base {
class FilePath;
class Time;
}

namespace content {
class BrowserContext;
}

namespace net {
class HttpResponseHeaders;
}

namespace extensions {
class InfoMap;

using ExtensionProtocolTestHandler =
    base::Callback<void(base::FilePath* directory_path,
                        base::FilePath* relative_path)>;

// Builds HTTP headers for an extension request. Hashes the time to avoid
// exposing the exact user installation time of the extension.
scoped_refptr<net::HttpResponseHeaders> BuildHttpHeaders(
    const std::string& content_security_policy,
    bool send_cors_header,
    const base::Time& last_modified_time);

// Creates the handlers for the chrome-extension:// scheme. Pass true for
// |is_incognito| only for incognito profiles and not for Chrome OS guest mode
// profiles.
std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler>
CreateExtensionProtocolHandler(bool is_incognito, InfoMap* extension_info_map);

// Allows tests to set a special handler for chrome-extension:// urls. Note
// that this goes through all the normal security checks; it's essentially a
// way to map extra resources to be included in extensions.
void SetExtensionProtocolTestHandler(ExtensionProtocolTestHandler* handler);

// Creates a new network::mojom::URLLoaderFactory implementation suitable for
// handling navigation requests to extension URLs.
std::unique_ptr<network::mojom::URLLoaderFactory>
CreateExtensionNavigationURLLoaderFactory(
    content::BrowserContext* browser_context,
    bool is_web_view_request);

// Creates a network::mojom::URLLoaderFactory implementation suitable for
// handling subresource requests for extension URLs for the frame identified by
// |render_process_id| and |render_frame_id|.
// This function can also be used to make a factory for other non-subresource
// requests to extension URLs, such as for the service worker script when
// starting a service worker. In that case, render_frame_id will be
// MSG_ROUTING_NONE.
std::unique_ptr<network::mojom::URLLoaderFactory>
CreateExtensionURLLoaderFactory(int render_process_id, int render_frame_id);

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_PROTOCOLS_H_
