// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_FORMAT_UTILS_H_
#define MEDIA_GPU_FORMAT_UTILS_H_

#include "media/base/video_types.h"
#include "media/gpu/media_gpu_export.h"
#include "ui/gfx/buffer_types.h"

namespace media {

MEDIA_GPU_EXPORT VideoPixelFormat
GfxBufferFormatToVideoPixelFormat(gfx::BufferFormat format);

MEDIA_GPU_EXPORT gfx::BufferFormat VideoPixelFormatToGfxBufferFormat(
    VideoPixelFormat pixel_format);

}  // namespace media

#endif  // MEDIA_GPU_FORMAT_UTILS_H_
