// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_COMMON_TEXTURE_IN_USE_RESPONSE_H_
#define GPU_COMMAND_BUFFER_COMMON_TEXTURE_IN_USE_RESPONSE_H_

#include <stdint.h>

#include <vector>

#include "gpu/gpu_export.h"

namespace gpu {

// A response from the gpu process about whether a texture is in use by the
// system compositor.
struct GPU_EXPORT TextureInUseResponse {
  uint32_t texture = 0;
  bool in_use = false;
};

using TextureInUseResponses = std::vector<TextureInUseResponse>;

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_COMMON_TEXTURE_IN_USE_RESPONSE_H_
