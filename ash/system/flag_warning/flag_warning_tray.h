// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_FLAG_WARNING_FLAG_WARNING_TRAY_H_
#define ASH_SYSTEM_FLAG_WARNING_FLAG_WARNING_TRAY_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace views {
class MdTextButton;
}

namespace ash {
class Shelf;
class TrayContainer;

// Adds an indicator to the status area if certain high-risk flags or features
// are enabled, for example mash. Clicking the button opens about:flags so the
// user can reset the flag. For consistency with other status area tray views,
// this view is always created but only made visible when the flag is set.
class ASH_EXPORT FlagWarningTray : public views::View,
                                   public views::ButtonListener {
 public:
  explicit FlagWarningTray(Shelf* shelf);
  ~FlagWarningTray() override;

  // Update the child view layout and appearance for horizontal or vertical
  // shelf alignments.
  void UpdateAfterShelfAlignmentChange();

  // views::View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

 private:
  // Update the button label and icon based on shelf state.
  void UpdateButton();

  const Shelf* const shelf_;

  // Owned by views hierarchy.
  TrayContainer* container_ = nullptr;
  views::MdTextButton* button_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(FlagWarningTray);
};

}  // namespace ash

#endif  // ASH_SYSTEM_FLAG_WARNING_FLAG_WARNING_TRAY_H_
