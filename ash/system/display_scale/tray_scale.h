// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_DISPLAY_SCALE_TRAY_SCALE_H_
#define ASH_SYSTEM_DISPLAY_SCALE_TRAY_SCALE_H_

#include <stdint.h>

#include "ash/ash_export.h"
#include "ash/system/tray/tray_image_item.h"
#include "base/macros.h"

namespace ash {
namespace tray {
class ScaleView;
class ScaleDetailedView;
}  // namespace tray

class DetailedViewDelegate;

// The system tray item for display scale.
class ASH_EXPORT TrayScale : public SystemTrayItem {
 public:
  explicit TrayScale(SystemTray* system_tray);
  ~TrayScale() override;

 private:
  // Overridden from SystemTrayItem.
  views::View* CreateDefaultView(LoginStatus status) override;
  views::View* CreateDetailedView(LoginStatus status) override;
  void OnDefaultViewDestroyed() override;
  void OnDetailedViewDestroyed() override;
  bool ShouldShowShelf() const override;

  tray::ScaleView* scale_view_;

  tray::ScaleDetailedView* scale_detail_view_;

  const std::unique_ptr<DetailedViewDelegate> detailed_view_delegate_;

  DISALLOW_COPY_AND_ASSIGN(TrayScale);
};

}  // namespace ash

#endif  // ASH_SYSTEM_DISPLAY_SCALE_TRAY_SCALE_H_
