// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_ID_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_ID_H_

#include <stdint.h>

#include "base/containers/flat_set.h"

namespace viz {

using ResourceId = uint32_t;
using ResourceIdSet = base::flat_set<ResourceId>;
constexpr ResourceId kInvalidResourceId = 0;

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_ID_H_
