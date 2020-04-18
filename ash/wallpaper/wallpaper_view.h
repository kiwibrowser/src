// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WALLPAPER_WALLPAPER_VIEW_H_
#define ASH_WALLPAPER_WALLPAPER_VIEW_H_

#include <memory>

#include "ui/views/context_menu_controller.h"
#include "ui/views/view.h"

namespace aura {
class Window;
}

namespace ash {

class PreEventDispatchHandler;

class WallpaperView : public views::View, public views::ContextMenuController {
 public:
  WallpaperView();
  ~WallpaperView() override;

 private:
  friend class WallpaperControllerTest;

  // Overridden from views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;

  // Overridden from views::ContextMenuController:
  void ShowContextMenuForView(views::View* source,
                              const gfx::Point& point,
                              ui::MenuSourceType source_type) override;
  std::unique_ptr<PreEventDispatchHandler> pre_dispatch_handler_;

  DISALLOW_COPY_AND_ASSIGN(WallpaperView);
};

views::Widget* CreateWallpaperWidget(aura::Window* root_window,
                                     int container_id);

}  // namespace ash

#endif  // ASH_WALLPAPER_WALLPAPER_VIEW_H_
