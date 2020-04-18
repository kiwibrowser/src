// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_CHROME_EXTENSIONS_INTERFACE_REGISTRATION_H_
#define CHROME_BROWSER_EXTENSIONS_CHROME_EXTENSIONS_INTERFACE_REGISTRATION_H_

#include "services/service_manager/public/cpp/binder_registry.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace extensions {

class Extension;

void RegisterChromeInterfacesForExtension(
    service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>*
        registry,
    content::RenderFrameHost* render_frame_host,
    const Extension* extension);

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_CHROME_EXTENSIONS_INTERFACE_REGISTRATION_H_
