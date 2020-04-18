// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_TYPE_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_TYPE_H_

namespace viz {

// Types of resources that can be sent to the viz compositing service.
enum class ResourceType {
  kTexture,
  kBitmap,
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_TYPE_H_
