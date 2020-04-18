// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_RESOURCE_TIMING_INFO_CONVERSIONS_H_
#define CONTENT_RENDERER_RESOURCE_TIMING_INFO_CONVERSIONS_H_

#include "content/common/resource_timing_info.h"
#include "third_party/blink/public/platform/web_resource_timing_info.h"

namespace content {

ResourceTimingInfo WebResourceTimingInfoToResourceTimingInfo(
    const blink::WebResourceTimingInfo& info);
blink::WebResourceTimingInfo ResourceTimingInfoToWebResourceTimingInfo(
    const ResourceTimingInfo& resource_timing);

}  // namespace content

#endif  // CONTENT_RENDERER_RESOURCE_TIMING_INFO_CONVERSIONS_H_
