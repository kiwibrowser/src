// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_SWITCHES_H_
#define COMPONENTS_VIZ_COMMON_SWITCHES_H_

#include <stdint.h>
#include <string>

#include "base/optional.h"
#include "components/viz/common/viz_common_export.h"

namespace switches {

// Keep list in alphabetical order.
VIZ_COMMON_EXPORT extern const char kDeadlineToSynchronizeSurfaces[];
VIZ_COMMON_EXPORT extern const char kEnableSurfaceSynchronization[];
VIZ_COMMON_EXPORT extern const char kEnableVizDevTools[];
VIZ_COMMON_EXPORT extern const char kRunAllCompositorStagesBeforeDraw[];
VIZ_COMMON_EXPORT extern const char kUseVizHitTestSurfaceLayer[];
VIZ_COMMON_EXPORT extern const char kDisableFrameRateLimit[];

VIZ_COMMON_EXPORT base::Optional<uint32_t> GetDeadlineToSynchronizeSurfaces();

}  // namespace switches

#endif  // COMPONENTS_VIZ_COMMON_SWITCHES_H_
