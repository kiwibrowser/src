// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/snapshot/snapshot_async.h"

#include "base/location.h"
#include "base/numerics/safe_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "skia/ext/image_operations.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/skbitmap_operations.h"

namespace ui {

namespace {

void OnFrameScalingFinished(const GrabWindowSnapshotAsyncCallback& callback,
                            const SkBitmap& scaled_bitmap) {
  callback.Run(gfx::Image::CreateFrom1xBitmap(scaled_bitmap));
}

SkBitmap ScaleBitmap(const SkBitmap& input_bitmap,
                     const gfx::Size& target_size) {
  return skia::ImageOperations::Resize(input_bitmap,
                                       skia::ImageOperations::RESIZE_GOOD,
                                       target_size.width(),
                                       target_size.height(),
                                       static_cast<SkBitmap::Allocator*>(NULL));
}

}  // namespace

void SnapshotAsync::ScaleCopyOutputResult(
    const GrabWindowSnapshotAsyncCallback& callback,
    const gfx::Size& target_size,
    std::unique_ptr<viz::CopyOutputResult> result) {
  const SkBitmap bitmap = result->AsSkBitmap();
  if (!bitmap.readyToDraw()) {
    callback.Run(gfx::Image());
    return;
  }

  // TODO(sergeyu): Potentially images can be scaled on GPU before reading it
  // from GPU. Image scaling is implemented in content::GlHelper, but it's can't
  // be used here because it's not in content/public. Move the scaling code
  // somewhere so that it can be reused here.
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::Bind(ScaleBitmap, bitmap, target_size),
      base::Bind(&OnFrameScalingFinished, callback));
}

void SnapshotAsync::RunCallbackWithCopyOutputResult(
    const GrabWindowSnapshotAsyncCallback& callback,
    std::unique_ptr<viz::CopyOutputResult> result) {
  const SkBitmap bitmap = result->AsSkBitmap();
  if (!bitmap.readyToDraw()) {
    callback.Run(gfx::Image());
    return;
  }
  callback.Run(gfx::Image::CreateFrom1xBitmap(bitmap));
}

}  // namespace ui
