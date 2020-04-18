// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/presentation_request.h"

namespace content {

PresentationRequest::PresentationRequest(
    const std::pair<int, int>& render_frame_host_id,
    const std::vector<GURL>& presentation_urls,
    const url::Origin& frame_origin)
    : render_frame_host_id(render_frame_host_id),
      presentation_urls(presentation_urls),
      frame_origin(frame_origin) {}

PresentationRequest::~PresentationRequest() = default;

PresentationRequest::PresentationRequest(const PresentationRequest& other) =
    default;

PresentationRequest& PresentationRequest::operator=(
    const PresentationRequest& other) = default;

}  // namespace content
