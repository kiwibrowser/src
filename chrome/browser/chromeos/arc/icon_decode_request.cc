// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/icon_decode_request.h"

#include <memory>
#include <utility>
#include <vector>

#include "chrome/grit/component_extension_resources.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/scale_factor.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/image/image_skia_rep.h"
#include "ui/gfx/image/image_skia_source.h"

using content::BrowserThread;

namespace arc {

namespace {

bool disable_safe_decoding_for_testing = false;

class IconSource : public gfx::ImageSkiaSource {
 public:
  IconSource(const SkBitmap& decoded_bitmap, int resource_size_in_dip);
  explicit IconSource(int resource_size_in_dip);
  ~IconSource() override = default;

  void SetDecodedImage(const SkBitmap& decoded_bitmap);

 private:
  gfx::ImageSkiaRep GetImageForScale(float scale) override;

  const int resource_size_in_dip_;
  gfx::ImageSkia decoded_icon_;

  DISALLOW_COPY_AND_ASSIGN(IconSource);
};

IconSource::IconSource(int resource_size_in_dip)
    : resource_size_in_dip_(resource_size_in_dip) {}

void IconSource::SetDecodedImage(const SkBitmap& decoded_bitmap) {
  decoded_icon_.AddRepresentation(gfx::ImageSkiaRep(
      decoded_bitmap, ui::GetScaleForScaleFactor(ui::SCALE_FACTOR_100P)));
}

gfx::ImageSkiaRep IconSource::GetImageForScale(float scale) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // We use the icon if it was decoded successfully, otherwise use the default
  // ARC icon.
  const gfx::ImageSkia* icon_to_scale;
  if (decoded_icon_.isNull()) {
    int resource_id =
        scale >= 1.5f ? IDR_ARC_SUPPORT_ICON_96 : IDR_ARC_SUPPORT_ICON_48;
    icon_to_scale =
        ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(resource_id);
  } else {
    icon_to_scale = &decoded_icon_;
  }
  DCHECK(icon_to_scale);

  gfx::ImageSkia resized_image = gfx::ImageSkiaOperations::CreateResizedImage(
      *icon_to_scale, skia::ImageOperations::RESIZE_BEST,
      gfx::Size(resource_size_in_dip_, resource_size_in_dip_));
  return resized_image.GetRepresentation(scale);
}

}  // namespace

// static
void IconDecodeRequest::DisableSafeDecodingForTesting() {
  disable_safe_decoding_for_testing = true;
}

IconDecodeRequest::IconDecodeRequest(SetIconCallback set_icon_callback,
                                     int requested_size)
    : set_icon_callback_(std::move(set_icon_callback)),
      requested_size_(requested_size) {}

IconDecodeRequest::~IconDecodeRequest() = default;

void IconDecodeRequest::StartWithOptions(
    const std::vector<uint8_t>& image_data) {
  if (disable_safe_decoding_for_testing) {
    if (image_data.empty()) {
      OnDecodeImageFailed();
      return;
    }
    SkBitmap bitmap;
    if (!gfx::PNGCodec::Decode(
            reinterpret_cast<const unsigned char*>(image_data.data()),
            image_data.size(), &bitmap)) {
      OnDecodeImageFailed();
      return;
    }
    OnImageDecoded(bitmap);
    return;
  }
  ImageDecoder::StartWithOptions(this, image_data, ImageDecoder::DEFAULT_CODEC,
                                 true, gfx::Size());
}

void IconDecodeRequest::OnImageDecoded(const SkBitmap& bitmap) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const gfx::Size resource_size(requested_size_, requested_size_);
  auto icon_source = std::make_unique<IconSource>(requested_size_);
  icon_source->SetDecodedImage(bitmap);
  const gfx::ImageSkia icon =
      gfx::ImageSkia(std::move(icon_source), resource_size);
  icon.EnsureRepsForSupportedScales();

  std::move(set_icon_callback_).Run(icon);
}

void IconDecodeRequest::OnDecodeImageFailed() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DLOG(ERROR) << "Failed to decode an icon image.";

  const gfx::Size resource_size(requested_size_, requested_size_);
  auto icon_source = std::make_unique<IconSource>(requested_size_);
  const gfx::ImageSkia icon =
      gfx::ImageSkia(std::move(icon_source), resource_size);
  icon.EnsureRepsForSupportedScales();

  std::move(set_icon_callback_).Run(icon);
}

}  // namespace arc
