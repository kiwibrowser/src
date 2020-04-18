// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RASTER_RASTER_BUFFER_H_
#define CC_RASTER_RASTER_BUFFER_H_

#include <stdint.h>

#include "cc/cc_export.h"
#include "cc/raster/raster_source.h"
#include "ui/gfx/geometry/rect.h"

namespace gfx {
class AxisTransform2d;
}  // namespace gfx

namespace cc {

class CC_EXPORT RasterBuffer {
 public:
  RasterBuffer();
  virtual ~RasterBuffer();

  virtual void Playback(
      const RasterSource* raster_source,
      const gfx::Rect& raster_full_rect,
      const gfx::Rect& raster_dirty_rect,
      uint64_t new_content_id,
      const gfx::AxisTransform2d& transform,
      const RasterSource::PlaybackSettings& playback_settings) = 0;
};

}  // namespace cc

#endif  // CC_RASTER_RASTER_BUFFER_H_
