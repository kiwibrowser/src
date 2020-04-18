// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray/tray_image_item.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_item_view.h"
#include "ash/system/tray/tray_utils.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

TrayImageItem::TrayImageItem(SystemTray* system_tray,
                             const gfx::VectorIcon& icon,
                             UmaType uma_type)
    : SystemTrayItem(system_tray, uma_type),
      icon_(icon),
      icon_color_(kTrayIconColor),
      tray_view_(nullptr) {}

TrayImageItem::~TrayImageItem() = default;

views::View* TrayImageItem::tray_view() {
  return tray_view_;
}

views::View* TrayImageItem::CreateTrayView(LoginStatus status) {
  CHECK(!tray_view_);
  tray_view_ = new TrayItemView(this);
  tray_view_->CreateImageView();
  SetImageIcon(icon_);
  tray_view_->SetVisible(GetInitialVisibility());
  return tray_view_;
}

void TrayImageItem::OnTrayViewDestroyed() {
  tray_view_ = nullptr;
}

void TrayImageItem::SetIconColor(SkColor color) {
  icon_color_ = color;
  SetImageIcon(icon_);
}

void TrayImageItem::SetImageIcon(const gfx::VectorIcon& icon) {
  if (!tray_view_)
    return;

  tray_view_->image_view()->SetImage(
      gfx::CreateVectorIcon(icon, kTrayIconSize, icon_color_));
}

}  // namespace ash
