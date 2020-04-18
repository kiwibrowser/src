// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wallpaper/wallpaper_utils/wallpaper_decoder.h"

#include "ash/shell.h"
#include "ash/shell_delegate.h"
#include "ipc/ipc_channel.h"
#include "services/data_decoder/public/cpp/decode_image.h"

namespace ash {
namespace {

const int64_t kMaxImageSizeInBytes =
    static_cast<int64_t>(IPC::Channel::kMaximumMessageSize);

void ConvertToImageSkia(OnWallpaperDecoded callback, const SkBitmap& image) {
  if (image.isNull()) {
    std::move(callback).Run(gfx::ImageSkia());
    return;
  }
  SkBitmap final_image = image;
  final_image.setImmutable();
  gfx::ImageSkia image_skia = gfx::ImageSkia::CreateFrom1xBitmap(final_image);
  image_skia.MakeThreadSafe();

  std::move(callback).Run(image_skia);
}

}  // namespace

void DecodeWallpaper(const std::string& image_data,
                     OnWallpaperDecoded callback) {
  // The connector for the mojo service manager is null in unit tests.
  if (!Shell::Get()->shell_delegate()->GetShellConnector()) {
    std::move(callback).Run(gfx::ImageSkia());
    return;
  }
  std::vector<uint8_t> image_bytes(image_data.begin(), image_data.end());
  data_decoder::DecodeImage(
      Shell::Get()->shell_delegate()->GetShellConnector(),
      std::move(image_bytes), data_decoder::mojom::ImageCodec::ROBUST_JPEG,
      false /* shrink_to_fit */, kMaxImageSizeInBytes,
      gfx::Size() /* desired_image_frame_size */,
      base::BindOnce(&ConvertToImageSkia, std::move(callback)));
}

}  // namespace ash