// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_WALLPAPER_STRUCT_TRAITS_H_
#define ASH_PUBLIC_CPP_WALLPAPER_STRUCT_TRAITS_H_

#include "ash/public/cpp/wallpaper_types.h"
#include "ash/public/interfaces/wallpaper.mojom.h"

namespace mojo {

template <>
struct EnumTraits<ash::mojom::WallpaperLayout, ash::WallpaperLayout> {
  static ash::mojom::WallpaperLayout ToMojom(ash::WallpaperLayout input) {
    switch (input) {
      case ash::WALLPAPER_LAYOUT_CENTER:
        return ash::mojom::WallpaperLayout::CENTER;
      case ash::WALLPAPER_LAYOUT_CENTER_CROPPED:
        return ash::mojom::WallpaperLayout::CENTER_CROPPED;
      case ash::WALLPAPER_LAYOUT_STRETCH:
        return ash::mojom::WallpaperLayout::STRETCH;
      case ash::WALLPAPER_LAYOUT_TILE:
        return ash::mojom::WallpaperLayout::TILE;
      case ash::NUM_WALLPAPER_LAYOUT:
        break;
    }
    NOTREACHED();
    return ash::mojom::WallpaperLayout::CENTER;
  }

  static bool FromMojom(ash::mojom::WallpaperLayout input,
                        ash::WallpaperLayout* out) {
    switch (input) {
      case ash::mojom::WallpaperLayout::CENTER:
        *out = ash::WALLPAPER_LAYOUT_CENTER;
        return true;
      case ash::mojom::WallpaperLayout::CENTER_CROPPED:
        *out = ash::WALLPAPER_LAYOUT_CENTER_CROPPED;
        return true;
      case ash::mojom::WallpaperLayout::STRETCH:
        *out = ash::WALLPAPER_LAYOUT_STRETCH;
        return true;
      case ash::mojom::WallpaperLayout::TILE:
        *out = ash::WALLPAPER_LAYOUT_TILE;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

}  // namespace mojo

#endif  // ASH_PUBLIC_CPP_WALLPAPER_STRUCT_TRAITS_H_
