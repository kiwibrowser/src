// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/thumbnail_decoder_impl.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace offline_pages {
namespace {

gfx::Image CropSquare(const gfx::Image& image) {
  if (image.IsEmpty())
    return image;

  const gfx::ImageSkia* skimage = image.ToImageSkia();
  gfx::Rect bounds{{0, 0}, skimage->size()};
  int size = std::min(bounds.width(), bounds.height());
  bounds.ClampToCenteredSize({size, size});
  return gfx::Image(gfx::ImageSkiaOperations::CreateTiledImage(
      *skimage, bounds.x(), bounds.y(), bounds.width(), bounds.height()));
}

}  // namespace

ThumbnailDecoderImpl::ThumbnailDecoderImpl(
    std::unique_ptr<image_fetcher::ImageDecoder> decoder)
    : image_decoder_(std::move(decoder)) {
  CHECK(image_decoder_);
}

ThumbnailDecoderImpl::~ThumbnailDecoderImpl() = default;

void ThumbnailDecoderImpl::DecodeAndCropThumbnail(
    const std::string& thumbnail_data,
    DecodeComplete complete_callback) {
  auto callback = base::BindOnce(
      [](ThumbnailDecoder::DecodeComplete complete_callback,
         const gfx::Image& image) {
        if (image.IsEmpty()) {
          std::move(complete_callback).Run(image);
          return;
        }
        std::move(complete_callback).Run(CropSquare(image));
      },
      std::move(complete_callback));

  image_decoder_->DecodeImage(
      thumbnail_data, gfx::Size(),
      base::AdaptCallbackForRepeating(std::move(callback)));
}

}  // namespace offline_pages
