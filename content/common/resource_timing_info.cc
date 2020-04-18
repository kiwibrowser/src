// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/resource_timing_info.h"

namespace content {

ServerTimingInfo::ServerTimingInfo() = default;
ServerTimingInfo::ServerTimingInfo(const ServerTimingInfo&) = default;
ServerTimingInfo::~ServerTimingInfo() = default;

ResourceLoadTiming::ResourceLoadTiming() = default;
ResourceLoadTiming::ResourceLoadTiming(const ResourceLoadTiming&) = default;
ResourceLoadTiming::~ResourceLoadTiming() = default;

ResourceTimingInfo::ResourceTimingInfo() = default;
ResourceTimingInfo::ResourceTimingInfo(const ResourceTimingInfo&) = default;
ResourceTimingInfo::~ResourceTimingInfo() = default;

}  // namespace content
