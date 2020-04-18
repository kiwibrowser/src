// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WALLPAPER_WALLPAPER_CONTROLLER_OBSERVER_H_
#define ASH_WALLPAPER_WALLPAPER_CONTROLLER_OBSERVER_H_

#include "ash/ash_export.h"

namespace ash {

class ASH_EXPORT WallpaperControllerObserver {
 public:
  // Invoked when the colors extracted from the current wallpaper change.
  virtual void OnWallpaperColorsChanged() {}

  // Invoked when the blur state of the wallpaper changes.
  virtual void OnWallpaperBlurChanged() {}

  // Invoked when the wallpaper preview mode starts.
  virtual void OnWallpaperPreviewStarted() {}

  // Invoked when the wallpaper preview mode ends.
  virtual void OnWallpaperPreviewEnded() {}

 protected:
  virtual ~WallpaperControllerObserver() {}
};

}  // namespace ash

#endif  // ASH_WALLPAPER_WALLPAPER_CONTROLLER_OBSERVER_H_
