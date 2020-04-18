// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_USER_BUTTON_FROM_VIEW_H_
#define ASH_SYSTEM_USER_BUTTON_FROM_VIEW_H_

#include <memory>

#include "ash/system/tray/tray_popup_ink_drop_style.h"
#include "base/macros.h"
#include "ui/views/controls/button/button.h"

namespace views {
class InkDropContainerView;
}  // namespace views

namespace ash {
namespace tray {

// This view is used to wrap it's content and transform it into button.
class ButtonFromView : public views::Button {
 public:
  // The |content| is the content which is shown within the button. The
  // |button_listener| will be informed - if provided - when a button was
  // pressed.
  ButtonFromView(views::View* content,
                 views::ButtonListener* listener,
                 TrayPopupInkDropStyle ink_drop_style);
  ~ButtonFromView() override;

  // views::View:
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void Layout() override;

  // Check if the item is hovered.
  bool is_hovered_for_test() { return button_hovered_; }

 protected:
  // views::Button:
  void AddInkDropLayer(ui::Layer* ink_drop_layer) override;
  void RemoveInkDropLayer(ui::Layer* ink_drop_layer) override;
  std::unique_ptr<views::InkDrop> CreateInkDrop() override;
  std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override;
  std::unique_ptr<views::InkDropHighlight> CreateInkDropHighlight()
      const override;
  std::unique_ptr<views::InkDropMask> CreateInkDropMask() const override;

 private:
  // Content of button.
  views::View* content_;

  // Defines the flavor of ink drop ripple/highlight that should be constructed.
  TrayPopupInkDropStyle ink_drop_style_;

  // True if button is hovered.
  bool button_hovered_;

  // A separate view is necessary to hold the ink drop layer so that |this| can
  // host labels with subpixel anti-aliasing enabled.
  views::InkDropContainerView* ink_drop_container_;

  std::unique_ptr<views::InkDropMask> ink_drop_mask_;

  DISALLOW_COPY_AND_ASSIGN(ButtonFromView);
};

}  // namespace tray
}  // namespace ash

#endif  // ASH_SYSTEM_USER_BUTTON_FROM_VIEW_H_
