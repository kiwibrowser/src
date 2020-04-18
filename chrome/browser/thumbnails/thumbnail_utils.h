// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_THUMBNAILS_THUMBNAIL_UTILS_H_
#define CHROME_BROWSER_THUMBNAILS_THUMBNAIL_UTILS_H_

#include "base/macros.h"
#include "ui/base/resource/scale_factor.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace thumbnails {

// The result of clipping. This can be used to determine if the
// generated thumbnail is good or not.
enum ClipResult {
  // Clipping is not done yet.
  CLIP_RESULT_UNPROCESSED,
  // The source image is smaller.
  CLIP_RESULT_SOURCE_IS_SMALLER,
  // Wider than tall by twice or more, clip horizontally.
  CLIP_RESULT_MUCH_WIDER_THAN_TALL,
  // Wider than tall, clip horizontally.
  CLIP_RESULT_WIDER_THAN_TALL,
  // Taller than wide, clip vertically.
  CLIP_RESULT_TALLER_THAN_WIDE,
  // The source and destination aspect ratios are identical.
  CLIP_RESULT_NOT_CLIPPED,
};

bool IsGoodClipping(ClipResult clip_result);

// The implementation of the 'classic' thumbnail cropping algorithm. It is not
// content-driven in any meaningful way (save for score calculation). Rather,
// the choice of a cropping region is based on relation between source and
// target sizes. The selected source region is then rescaled into the target
// thumbnail image.

// Provides information necessary to crop-and-resize image data from a source
// canvas of |source_size|. Auxiliary |scale_factor| helps compute the target
// thumbnail size to be copied from the backing store, in pixels. Parameters
// of the required copy operation are assigned to |clipping_rect| (cropping
// rectangle for the source canvas) and |copy_size| (the size of the copied
// bitmap in pixels). The return value indicates the type of clipping that
// will be done.
ClipResult GetCanvasCopyInfo(const gfx::Size& source_size,
                             ui::ScaleFactor scale_factor,
                             const gfx::Size& target_size,
                             gfx::Rect* clipping_rect,
                             gfx::Size* copy_size);

// Returns the size copied from the backing store. |thumbnail_size| is in
// DIP, returned size in pixels.
gfx::Size GetCopySizeForThumbnail(ui::ScaleFactor scale_factor,
                                  const gfx::Size& thumbnail_size);
gfx::Rect GetClippingRect(const gfx::Size& source_size,
                          const gfx::Size& desired_size,
                          ClipResult* clip_result);

}  // namespace thumbnails

#endif  // CHROME_BROWSER_THUMBNAILS_THUMBNAIL_UTILS_H_
