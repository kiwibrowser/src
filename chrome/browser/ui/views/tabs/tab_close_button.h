// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TABS_TAB_CLOSE_BUTTON_H_
#define CHROME_BROWSER_UI_VIEWS_TABS_TAB_CLOSE_BUTTON_H_

#include "base/callback_forward.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/masked_targeter_delegate.h"

class Tab;

// This is a Button subclass that shows the tab closed icon.
//
// In addition to setup for the icon, it forwards middle clicks to the parent
// View by explicitly not handling them in OnMousePressed.
class TabCloseButton : public views::ImageButton,
                       public views::MaskedTargeterDelegate {
 public:
  using MouseEventCallback =
      base::Callback<void(views::View*, const ui::MouseEvent&)>;

  // The mouse_event callback will be called for every mouse event to allow
  // middle clicks to be handled by the parent.
  //
  // See note on SetTabColor.
  TabCloseButton(views::ButtonListener* listener,
                 MouseEventCallback mouse_event_callback);
  ~TabCloseButton() override;

  // This function must be called before the tab is painted so it knows what
  // color to use. It must also be called when the background color of the tab
  // changes (this class does not track tab activation state), and when the
  // theme changes. |tab_color_is_dark| will be true if the tab is a dark
  // color. This will NOT be called when in newer material ui mode.
  void SetTabColor(SkColor color, bool tab_color_is_dark);

  // This is called whenever the |parent_tab| changes its active state. This
  // is only called when in newer material ui mode.
  void ActiveStateChanged(const Tab* parent_tab);

  // views::View:
  View* GetTooltipHandlerForPoint(const gfx::Point& point) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseMoved(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  const char* GetClassName() const override;

 protected:
  void PaintButtonContents(gfx::Canvas* canvas) override;

 private:
  // views::MaskedTargeterDelegate:
  views::View* TargetForRect(views::View* root, const gfx::Rect& rect) override;
  bool GetHitTestMask(gfx::Path* mask) const override;

  // In material refresh mode, calculates opacity based on the current state of
  // the hover animation on the parent tab.
  SkAlpha GetOpacity();

  void GenerateImages(bool is_touch,
                      SkColor normal_icon_color,
                      SkColor hover_pressed_icon_color,
                      SkColor hover_highlight_color,
                      SkColor pressed_highlight_color);

  MouseEventCallback mouse_event_callback_;

  DISALLOW_COPY_AND_ASSIGN(TabCloseButton);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TABS_TAB_CLOSE_BUTTON_H_
