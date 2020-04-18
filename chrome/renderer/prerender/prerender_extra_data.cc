// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/prerender/prerender_extra_data.h"

#include "base/logging.h"

namespace prerender {

PrerenderExtraData::PrerenderExtraData(
    int prerender_id,
    int render_view_route_id,
    const gfx::Size& size)
    : prerender_id_(prerender_id),
      render_view_route_id_(render_view_route_id),
      size_(size) {
}

PrerenderExtraData::~PrerenderExtraData() {
}

// static
const PrerenderExtraData& PrerenderExtraData::FromPrerender(
    const blink::WebPrerender& prerender) {
  DCHECK(prerender.GetExtraData());
  return static_cast<const PrerenderExtraData&>(*prerender.GetExtraData());
}

}  // namespace prerender

