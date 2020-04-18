// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TILES_TRAY_TILES_H_
#define ASH_SYSTEM_TILES_TRAY_TILES_H_

#include "ash/ash_export.h"
#include "ash/system/tray/system_tray_item.h"
#include "base/macros.h"

namespace ash {
class SystemTray;
class TilesDefaultView;

// The tray item for the 'tiles' at the bottom of the system menu. Each tile is
// an image button which does not reqire explanatory text, and thus does not
// require its own dedicated row in the system menu.
class ASH_EXPORT TrayTiles : public SystemTrayItem {
 public:
  explicit TrayTiles(SystemTray* system_tray);
  ~TrayTiles() override;

  // Accessor needed to obtain the help button view for the first-run flow.
  views::View* GetHelpButtonView() const;

  TilesDefaultView* GetDefaultViewForTesting() const;
  views::View* CreateDefaultViewForTesting();

 private:
  friend class TrayTilesTest;

  // SystemTrayItem:
  views::View* CreateDefaultView(LoginStatus status) override;
  void OnDefaultViewDestroyed() override;

  TilesDefaultView* default_view_;

  DISALLOW_COPY_AND_ASSIGN(TrayTiles);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TILES_TRAY_TILES_H_
