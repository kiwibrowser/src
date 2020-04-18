// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_INTERFACE_BINDERS_H_
#define CONTENT_BROWSER_RENDERER_INTERFACE_BINDERS_H_

#include <string>

#include "mojo/public/cpp/system/message_pipe.h"

namespace url {
class Origin;
}

namespace content {
class RenderFrameHost;
class RenderProcessHost;

// Bind an interface request |interface_pipe| for |interface_name| received from
// a web worker with origin |origin| hosted in the renderer |host|.
void BindWorkerInterface(const std::string& interface_name,
                         mojo::ScopedMessagePipeHandle interface_pipe,
                         RenderProcessHost* host,
                         const url::Origin& origin);

// Try binding an interface request |interface_pipe| for |interface_name|
// received from |frame|.
bool TryBindFrameInterface(const std::string& interface_name,
                           mojo::ScopedMessagePipeHandle* interface_pipe,
                           RenderFrameHost* frame);

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_INTERFACE_BINDERS_H_
