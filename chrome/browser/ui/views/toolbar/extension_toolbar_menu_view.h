// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TOOLBAR_EXTENSION_TOOLBAR_MENU_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_TOOLBAR_EXTENSION_TOOLBAR_MENU_VIEW_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar_observer.h"
#include "ui/views/controls/scroll_view.h"

class AppMenu;
class Browser;
class BrowserActionsContainer;
class ToolbarActionsBar;

namespace views {
class MenuItemView;
}

// ExtensionToolbarMenuView is the view containing the extension actions that
// overflowed from the BrowserActionsContainer, and is contained in and owned by
// the app menu.
// In the event that the app menu was opened for an Extension Action drag-and-
// drop, this will also close the menu upon completion.
class ExtensionToolbarMenuView : public views::ScrollView,
                                 public ToolbarActionsBarObserver {
 public:
  ExtensionToolbarMenuView(Browser* browser,
                           AppMenu* app_menu,
                           views::MenuItemView* menu_item);
  ~ExtensionToolbarMenuView() override;

  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  int GetHeightForWidth(int width) const override;
  void Layout() override;

  BrowserActionsContainer* container_for_testing() {
    return container_;
  }

  // Sets the time delay the app menu takes to close after a drag-and-drop
  // operation.
  static void set_close_menu_delay_for_testing(int delay);

 private:
  // ToolbarActionsBarObserver:
  void OnToolbarActionsBarDestroyed() override;
  void OnToolbarActionDragDone() override;
  void OnToolbarActionsBarDidStartResize() override;

  // Closes the |app_menu_|.
  void CloseAppMenu();

  // Resizes and lays out the view.
  void Redraw();

  // Returns the padding before the BrowserActionsContainer in the menu.
  int start_padding() const;

  // The associated browser.
  Browser* const browser_;

  // The app menu, which may need to be closed after a drag-and-drop.
  AppMenu* app_menu_;

  // The MenuItemView this view is contained within.
  views::MenuItemView* menu_item_;

  // The overflow BrowserActionsContainer which is nested in this view.
  BrowserActionsContainer* container_ = nullptr;

  // The maximum allowed height for the view.
  int max_height_ = 0;

  ScopedObserver<ToolbarActionsBar, ToolbarActionsBarObserver>
      toolbar_actions_bar_observer_;

  base::WeakPtrFactory<ExtensionToolbarMenuView> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionToolbarMenuView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TOOLBAR_EXTENSION_TOOLBAR_MENU_VIEW_H_
