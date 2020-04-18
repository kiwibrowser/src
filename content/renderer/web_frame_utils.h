// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_WEB_FRAME_UTILS_H_
#define CONTENT_RENDERER_WEB_FRAME_UTILS_H_

namespace blink {
class WebFrame;
}

namespace content {

// Returns either a WebLocalFrame or WebRemoteFrame based on |routing_id|.
// Returns nullptr if |routing_id| doesn't properly map to a frame.
blink::WebFrame* GetWebFrameFromRoutingIdForFrameOrProxy(int routing_id);

}  // namespace content

#endif  // CONTENT_RENDERER_WEB_FRAME_UTILS_H_
