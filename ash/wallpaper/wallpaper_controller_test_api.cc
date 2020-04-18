// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wallpaper/wallpaper_controller_test_api.h"
#include "ash/wallpaper/wallpaper_controller.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"

namespace ash {

namespace {

const WallpaperInfo kTestWallpaperInfo = {"", WALLPAPER_LAYOUT_CENTER, DEFAULT,
                                          base::Time::Now().LocalMidnight()};

gfx::ImageSkia CreateImageWithColor(const SkColor color) {
  gfx::Canvas canvas(gfx::Size(5, 5), 1.0f, true);
  canvas.DrawColor(color);
  return gfx::ImageSkia::CreateFrom1xBitmap(canvas.GetBitmap());
}

}  // namespace

WallpaperControllerTestApi::WallpaperControllerTestApi(
    WallpaperController* controller)
    : controller_(controller) {}

WallpaperControllerTestApi::~WallpaperControllerTestApi() = default;

SkColor WallpaperControllerTestApi::ApplyColorProducingWallpaper() {
  controller_->ShowWallpaperImage(
      CreateImageWithColor(SkColorSetRGB(60, 40, 40)), kTestWallpaperInfo,
      false /*preview_mode=*/);
  return SkColorSetRGB(18, 12, 12);
}

void WallpaperControllerTestApi::StartWallpaperPreview() {
  // Preview mode is considered active when the two callbacks have non-empty
  // values. Their specific values don't matter for testing purpose.
  controller_->confirm_preview_wallpaper_callback_ =
      base::BindOnce(&WallpaperController::SetWallpaperFromInfo,
                     controller_->weak_factory_.GetWeakPtr(),
                     AccountId::FromUserEmail("user@test.com"),
                     user_manager::USER_TYPE_REGULAR, kTestWallpaperInfo,
                     true /*show_wallpaper=*/);
  controller_->reload_preview_wallpaper_callback_ =
      base::BindRepeating(&WallpaperController::ShowWallpaperImage,
                          controller_->weak_factory_.GetWeakPtr(),
                          CreateImageWithColor(SK_ColorBLUE),
                          kTestWallpaperInfo, true /*preview_mode=*/);
  // Show the preview wallpaper.
  controller_->reload_preview_wallpaper_callback_.Run();
}

void WallpaperControllerTestApi::EndWallpaperPreview(
    bool confirm_preview_wallpaper) {
  if (confirm_preview_wallpaper)
    controller_->ConfirmPreviewWallpaper();
  else
    controller_->CancelPreviewWallpaper();
}

}  // namespace ash
