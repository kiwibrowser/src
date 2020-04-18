// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_BUTTON_H_
#define CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_BUTTON_H_

#include <memory>

#include "base/macros.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"

class Profile;

namespace test {
class ToolbarButtonTestApi;
}

namespace ui {
class MenuModel;
}

namespace views {
class MenuModelAdapter;
class MenuRunner;
}

// This class provides basic drawing and mouse-over behavior for buttons
// appearing in the toolbar.
// TODO: Consider making ToolbarButton and AppMenuButton share a common base
// class https://crbug.com/819854.
class ToolbarButton : public views::ImageButton,
                      public views::ContextMenuController {
 public:
  // The profile and listener pointers must outlive this class. The model can
  // be null if no menu is to be shown.
  ToolbarButton(Profile* profile,
                views::ButtonListener* listener,
                std::unique_ptr<ui::MenuModel> model);
  ~ToolbarButton() override;

  // Set up basic mouseover border behavior.
  // Should be called before first paint.
  void Init();

  // Sets |margin_leading_| when the browser is maximized and updates layout
  // to make the focus rectangle centered.
  void SetLeadingMargin(int margin);

  // Methods for handling ButtonDropDown-style menus.
  void ClearPendingMenu();
  bool IsMenuShowing() const;

  // views::ImageButton:
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  // Showing the drop down results in a MouseCaptureLost, we need to ignore it.
  void OnMouseCaptureLost() override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  std::unique_ptr<views::InkDrop> CreateInkDrop() override;
  std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override;
  std::unique_ptr<views::InkDropHighlight> CreateInkDropHighlight()
      const override;
  std::unique_ptr<views::InkDropMask> CreateInkDropMask() const override;
  SkColor GetInkDropBaseColor() const override;

  // views::ContextMenuController:
  void ShowContextMenuForView(View* source,
                              const gfx::Point& point,
                              ui::MenuSourceType source_type) override;

 protected:
  // Returns if menu should be shown. Override this to change default behavior.
  virtual bool ShouldShowMenu();

  // Function to show the dropdown menu.
  virtual void ShowDropDownMenu(ui::MenuSourceType source_type);

 private:
  friend test::ToolbarButtonTestApi;

  // Callback for MenuModelAdapter.
  void OnMenuClosed();

  // views::ImageButton:
  const char* GetClassName() const override;

  // The associated profile. The browser theme affects rendering.
  Profile* profile_;

  // The model that populates the attached menu.
  std::unique_ptr<ui::MenuModel> model_;

  // Indicates if menu is currently showing.
  bool menu_showing_ = false;

  // Y position of mouse when left mouse button is pressed.
  int y_position_on_lbuttondown_ = 0;

  // The model adapter for the drop down menu.
  std::unique_ptr<views::MenuModelAdapter> menu_model_adapter_;

  // Menu runner to display drop down menu.
  std::unique_ptr<views::MenuRunner> menu_runner_;

  // A factory for tasks that show the dropdown context menu for the button.
  base::WeakPtrFactory<ToolbarButton> show_menu_factory_;

  DISALLOW_COPY_AND_ASSIGN(ToolbarButton);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_BUTTON_H_
