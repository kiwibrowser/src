// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/tiles/image_decode_cache.h"

#include "base/metrics/histogram_macros.h"
#include "cc/raster/tile_task.h"

namespace cc {

ImageDecodeCache::TaskResult::TaskResult(bool need_unref)
    : need_unref(need_unref) {}

ImageDecodeCache::TaskResult::TaskResult(scoped_refptr<TileTask> task)
    : task(std::move(task)), need_unref(true) {}

ImageDecodeCache::TaskResult::TaskResult(const TaskResult& result) = default;

ImageDecodeCache::TaskResult::~TaskResult() = default;

void ImageDecodeCache::RecordImageMipLevelUMA(int mip_level) {
  DCHECK_GE(mip_level, 0);
  DCHECK_LT(mip_level, 32);
  UMA_HISTOGRAM_EXACT_LINEAR("Renderer4.ImageDecodeMipLevel", mip_level + 1,
                             33);
}

}  // namespace cc
