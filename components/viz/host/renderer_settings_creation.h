// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_HOST_RENDERER_SETTINGS_CREATION_H_
#define COMPONENTS_VIZ_HOST_RENDERER_SETTINGS_CREATION_H_

#include <stdint.h>

#include "components/viz/host/viz_host_export.h"

namespace viz {
class RendererSettings;
}  // namespace viz

namespace viz {

VIZ_HOST_EXPORT RendererSettings CreateRendererSettings();

}  // namespace viz

#endif  // COMPONENTS_VIZ_HOST_RENDERER_SETTINGS_CREATION_H_
