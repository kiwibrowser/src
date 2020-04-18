// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_TRAY_IMAGE_ITEM_H_
#define ASH_SYSTEM_TRAY_TRAY_IMAGE_ITEM_H_

#include "ash/ash_export.h"
#include "ash/system/tray/system_tray_item.h"
#include "base/macros.h"
#include "third_party/skia/include/core/SkColor.h"

namespace views {
class ImageView;
}

namespace gfx {
struct VectorIcon;
}

namespace ash {
class TrayItemView;

// A system tray item that uses an image as its "tray view" in the status area.
class ASH_EXPORT TrayImageItem : public SystemTrayItem {
 public:
  TrayImageItem(SystemTray* system_tray,
                const gfx::VectorIcon& icon,
                UmaType uma_type);
  ~TrayImageItem() override;

  views::View* tray_view();

 protected:
  virtual bool GetInitialVisibility() = 0;

  // Overridden from SystemTrayItem.
  views::View* CreateTrayView(LoginStatus status) override;
  void OnTrayViewDestroyed() override;

  // Sets the color of the icon to |color|.
  void SetIconColor(SkColor color);

  // Sets showing |icon| on |tray_view_|'s ImageView.
  void SetImageIcon(const gfx::VectorIcon& icon);

 private:
  // The icon and its current color.
  const gfx::VectorIcon& icon_;
  SkColor icon_color_;

  // The image view in the tray.
  TrayItemView* tray_view_;

  DISALLOW_COPY_AND_ASSIGN(TrayImageItem);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_TRAY_IMAGE_ITEM_H_
