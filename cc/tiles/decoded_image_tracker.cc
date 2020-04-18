// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/tiles/decoded_image_tracker.h"
#include "base/trace_event/trace_event.h"

namespace cc {
namespace {
const int kNumFramesToLock = 2;
}  // namespace

DecodedImageTracker::DecodedImageTracker(ImageController* controller)
    : image_controller_(controller) {
  DCHECK(image_controller_);
}

DecodedImageTracker::~DecodedImageTracker() {
  for (auto& pair : locked_images_)
    image_controller_->UnlockImageDecode(pair.first);
}

void DecodedImageTracker::QueueImageDecode(
    const PaintImage& image,
    const gfx::ColorSpace& target_color_space,
    const base::Callback<void(bool)>& callback) {
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
               "DecodedImageTracker::QueueImageDecode", "frame_key",
               image.GetKeyForFrame(image.frame_index()).ToString());
  DCHECK(image_controller_);
  // Queue the decode in the image controller, but switch out the callback for
  // our own.
  auto image_bounds = SkIRect::MakeWH(image.width(), image.height());
  DrawImage draw_image(image, image_bounds, kNone_SkFilterQuality,
                       SkMatrix::I(), image.frame_index(), target_color_space);
  image_controller_->QueueImageDecode(
      draw_image, base::Bind(&DecodedImageTracker::ImageDecodeFinished,
                             base::Unretained(this), callback));
}

void DecodedImageTracker::NotifyFrameFinished() {
  // Go through the images and if the frame ref count goes to 0, unlock the
  // image in the controller.
  for (auto it = locked_images_.begin(); it != locked_images_.end();) {
    auto id = it->first;
    int& ref_count = it->second;
    if (--ref_count != 0) {
      ++it;
      continue;
    }
    image_controller_->UnlockImageDecode(id);
    it = locked_images_.erase(it);
  }
}

void DecodedImageTracker::ImageDecodeFinished(
    const base::Callback<void(bool)>& callback,
    ImageController::ImageDecodeRequestId id,
    ImageController::ImageDecodeResult result) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
               "DecodedImageTracker::ImageDecodeFinished");

  if (result == ImageController::ImageDecodeResult::SUCCESS)
    locked_images_.push_back(std::make_pair(id, kNumFramesToLock));
  bool decode_succeeded =
      result == ImageController::ImageDecodeResult::SUCCESS ||
      result == ImageController::ImageDecodeResult::DECODE_NOT_REQUIRED;
  callback.Run(decode_succeeded);
}

}  // namespace cc
