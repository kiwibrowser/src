// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RENDER_FRAME_HOST_ID_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RENDER_FRAME_HOST_ID_H_

#include <utility>

#include "base/hash.h"

namespace media_router {

using RenderFrameHostId = std::pair<int, int>;

struct RenderFrameHostIdHasher {
  std::size_t operator()(const RenderFrameHostId id) const {
    return base::HashInts(id.first, id.second);
  }
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RENDER_FRAME_HOST_ID_H_
