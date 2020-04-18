// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_MOJO_SERVICE_REGISTRATION_H_
#define EXTENSIONS_BROWSER_MOJO_SERVICE_REGISTRATION_H_

#include "services/service_manager/public/cpp/binder_registry.h"

namespace content {
class RenderFrameHost;
}

namespace extensions {

class Extension;

void RegisterInterfacesForExtension(service_manager::BinderRegistryWithArgs<
                                        content::RenderFrameHost*>* registry,
                                    content::RenderFrameHost* render_frame_host,
                                    const Extension* extension);

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_MOJO_SERVICE_REGISTRATION_H_
