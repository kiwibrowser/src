// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_TRAY_ITEM_MORE_H_
#define ASH_SYSTEM_TRAY_TRAY_ITEM_MORE_H_

#include <memory>

#include "ash/system/tray/actionable_view.h"
#include "base/macros.h"
#include "ui/views/view.h"

namespace views {
class ImageView;
class Label;
class View;
}

namespace ash {
class SystemTrayItem;
class TrayPopupItemStyle;
class TriView;

// A view with a more arrow on the right edge. Clicking on the view brings up
// the detailed view of the tray-item that owns it. If the view is disabled, it
// will not show the more arrow.
class TrayItemMore : public ActionableView {
 public:
  explicit TrayItemMore(SystemTrayItem* owner);
  ~TrayItemMore() override;

  void SetLabel(const base::string16& label);
  void SetImage(const gfx::ImageSkia& image_skia);

 protected:
  // Returns a style that will be applied to the elements in the UpdateStyle()
  // method if |this| is enabled; otherwise, we force |this| to use
  // ColorStyle::DISABLED.
  std::unique_ptr<TrayPopupItemStyle> CreateStyle() const;

  // Called by CreateStyle() to give descendants a chance to customize the
  // style; e.g. to change the style's ColorStyle based on whether Bluetooth is
  // enabled/disabled.
  virtual std::unique_ptr<TrayPopupItemStyle> HandleCreateStyle() const;

  // Applies the style created from CreateStyle(). Should be called whenever any
  // input state changes that changes the style configuration created by
  // CreateStyle(). E.g. if Bluetooth is changed between enabled/disabled then
  // a differently configured style will be returned from CreateStyle() and thus
  // it will need to be applied.
  virtual void UpdateStyle();

 private:
  // Overridden from ActionableView.
  bool PerformAction(const ui::Event& event) override;

  // Overridden from views::View.
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void OnEnabledChanged() override;
  void OnNativeThemeChanged(const ui::NativeTheme* theme) override;

  TriView* tri_view_;
  views::ImageView* icon_;
  views::Label* label_;
  views::ImageView* more_;

  DISALLOW_COPY_AND_ASSIGN(TrayItemMore);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_TRAY_ITEM_MORE_H_
