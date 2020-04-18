// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBUI_WEB_UI_URL_LOADER_FACTORY_INTERNAL_H_
#define CONTENT_BROWSER_WEBUI_WEB_UI_URL_LOADER_FACTORY_INTERNAL_H_

#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace content {
class RenderFrameHost;

// Creates a URLLoaderFactory interface pointer for serving WebUI requests to
// the given |render_frame_host|. The factory will only create loaders for
// requests with the same scheme as |scheme|. This is needed because there is
// more than one scheme used for WebUI, and not all have WebUI bindings.
network::mojom::URLLoaderFactoryPtr CreateWebUIURLLoaderBinding(
    RenderFrameHost* render_frame_host,
    const std::string& scheme);

}  // namespace content

#endif  // CONTENT_BROWSER_WEBUI_WEB_UI_URL_LOADER_FACTORY_INTERNAL_H_
