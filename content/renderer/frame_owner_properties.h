// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_FRAME_OWNER_PROPERTIES_H_
#define CONTENT_RENDERER_FRAME_OWNER_PROPERTIES_H_

#include "content/common/frame_owner_properties.h"
#include "third_party/blink/public/web/web_frame_owner_properties.h"

namespace content {

FrameOwnerProperties ConvertWebFrameOwnerPropertiesToFrameOwnerProperties(
    const blink::WebFrameOwnerProperties& web_frame_owner_properties);

blink::WebFrameOwnerProperties
ConvertFrameOwnerPropertiesToWebFrameOwnerProperties(
    const FrameOwnerProperties& frame_owner_properties);

}  // namespace content

#endif  // CONTENT_RENDERER_FRAME_OWNER_PROPERTIES_H_
