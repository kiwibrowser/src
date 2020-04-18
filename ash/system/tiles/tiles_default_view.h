// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TILES_TILES_DEFAULT_VIEW_H_
#define ASH_SYSTEM_TILES_TILES_DEFAULT_VIEW_H_

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace views {
class Button;
}

namespace ash {
class NightLightToggleButton;
class SystemTrayItem;

// The container view for the tiles in the bottom row of the system menu
// (settings, help, lock, and power).
class ASH_EXPORT TilesDefaultView : public views::View,
                                    public views::ButtonListener {
 public:
  explicit TilesDefaultView(SystemTrayItem* owner);
  ~TilesDefaultView() override;

  // Sets the layout manager and child views of |this|.
  // TODO(tdanderson|bruthig): Consider moving the layout manager
  // setup code to a location which can be shared by other system menu rows.
  void Init();

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // Accessor needed to obtain the help button view for the first-run flow.
  views::View* GetHelpButtonView() const;

  const views::Button* GetShutdownButtonViewForTest() const;

 private:
  friend class TrayTilesTest;

  SystemTrayItem* owner_;

  // Pointers to the child buttons of |this|. Note that some buttons may not
  // exist (depending on the user's current login status, for instance), in
  // which case the corresponding pointer will be null.
  views::Button* settings_button_;
  views::Button* help_button_;
  NightLightToggleButton* night_light_button_;
  views::Button* lock_button_;
  views::Button* power_button_;

  DISALLOW_COPY_AND_ASSIGN(TilesDefaultView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TILES_TILES_DEFAULT_VIEW_H_
