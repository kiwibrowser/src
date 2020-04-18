// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/web_frame_utils.h"

#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_frame_proxy.h"
#include "ipc/ipc_message.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_remote_frame.h"

namespace content {

blink::WebFrame* GetWebFrameFromRoutingIdForFrameOrProxy(int routing_id) {
  auto* render_frame = RenderFrameImpl::FromRoutingID(routing_id);
  if (render_frame)
    return render_frame->GetWebFrame();

  auto* render_frame_proxy = RenderFrameProxy::FromRoutingID(routing_id);
  if (render_frame_proxy)
    return render_frame_proxy->web_frame();

  return nullptr;
}

}  // namespace content
