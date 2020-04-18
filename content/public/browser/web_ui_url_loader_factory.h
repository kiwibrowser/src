// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_WEB_UI_URL_LOADER_FACTORY_H_
#define CONTENT_PUBLIC_BROWSER_WEB_UI_URL_LOADER_FACTORY_H_

#include <memory>
#include <string>

#include "base/containers/flat_set.h"
#include "content/common/content_export.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace content {
class RenderFrameHost;

// Create a URLLoaderFactory for loading resources matching the specified
// |scheme| and also from a "pseudo host" matching one in |allowed_hosts|.
CONTENT_EXPORT std::unique_ptr<network::mojom::URLLoaderFactory>
CreateWebUIURLLoader(RenderFrameHost* render_frame_host,
                     const std::string& scheme,
                     base::flat_set<std::string> allowed_hosts);

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_WEB_UI_URL_LOADER_FACTORY_H_
