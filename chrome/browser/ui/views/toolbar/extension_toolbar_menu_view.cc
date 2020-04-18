// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/toolbar/extension_toolbar_menu_view.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/app_menu.h"
#include "chrome/browser/ui/views/toolbar/browser_actions_container.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/submenu_view.h"

namespace {
// The delay before we close the app menu if this was opened for a drop so that
// the user can see a browser action if one was moved.
// This can be changed for tests.
int g_close_menu_delay = 300;
}

ExtensionToolbarMenuView::ExtensionToolbarMenuView(
    Browser* browser,
    AppMenu* app_menu,
    views::MenuItemView* menu_item)
    : browser_(browser),
      app_menu_(app_menu),
      menu_item_(menu_item),
      toolbar_actions_bar_observer_(this),
      weak_factory_(this) {
  // Use a transparent background so that the menu's background shows through.
  // None of the children use layers, so this should be ok.
  SetBackgroundColor(SK_ColorTRANSPARENT);
  BrowserActionsContainer* main =
      BrowserView::GetBrowserViewForBrowser(browser_)
          ->toolbar_button_provider()
          ->GetBrowserActionsContainer();
  container_ = new BrowserActionsContainer(browser_, main, main->delegate());
  SetContents(container_);

  // Listen for the drop to finish so we can close the app menu, if necessary.
  toolbar_actions_bar_observer_.Add(main->toolbar_actions_bar());

  // In *very* extreme cases, it's possible that there are so many overflowed
  // actions, we won't be able to show them all. Cap the height so that the
  // overflow won't be excessively tall (at 8 icons per row, this allows for
  // 104 total extensions).
  constexpr int kMaxOverflowRows = 13;
  max_height_ = container_->GetToolbarActionSize().height() * kMaxOverflowRows;
  ClipHeightTo(0, max_height_);
}

ExtensionToolbarMenuView::~ExtensionToolbarMenuView() {
}

gfx::Size ExtensionToolbarMenuView::CalculatePreferredSize() const {
  gfx::Size s = views::ScrollView::CalculatePreferredSize();
  // views::ScrollView::CalculatePreferredSize() includes the contents' size,
  // but not the scrollbar width. Add it in if necessary.
  if (container_->GetPreferredSize().height() > max_height_)
    s.Enlarge(GetScrollBarLayoutWidth(), 0);
  return s;
}

int ExtensionToolbarMenuView::GetHeightForWidth(int width) const {
  // The width passed in here includes the full width of the menu, so we need
  // to omit the necessary padding.
  const views::MenuConfig& menu_config = views::MenuConfig::instance();
  int end_padding = menu_config.arrow_to_edge_padding -
      container_->toolbar_actions_bar()->platform_settings().item_spacing;
  width -= start_padding() + end_padding;

  return views::ScrollView::GetHeightForWidth(width);
}

void ExtensionToolbarMenuView::Layout() {
  SetPosition(gfx::Point(start_padding(), 0));
  SizeToPreferredSize();
  views::ScrollView::Layout();
}

void ExtensionToolbarMenuView::set_close_menu_delay_for_testing(int delay) {
  g_close_menu_delay = delay;
}

void ExtensionToolbarMenuView::OnToolbarActionsBarDestroyed() {
  toolbar_actions_bar_observer_.RemoveAll();
}

void ExtensionToolbarMenuView::OnToolbarActionDragDone() {
  // In the case of a drag-and-drop, the bounds of the container may have
  // changed (in the case of removing an icon that was the last in a row).
  Redraw();

  // We need to close the app menu if it was just opened for the drag and drop,
  // or if there are no more extensions in the overflow menu after a drag and
  // drop.
  if (app_menu_->for_drop() ||
      container_->toolbar_actions_bar()->GetIconCount() == 0) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&ExtensionToolbarMenuView::CloseAppMenu,
                       weak_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(g_close_menu_delay));
  }
}

void ExtensionToolbarMenuView::OnToolbarActionsBarDidStartResize() {
  Redraw();
}

void ExtensionToolbarMenuView::CloseAppMenu() {
  app_menu_->CloseMenu();
}

void ExtensionToolbarMenuView::Redraw() {
  // In a case where the size of the container may have changed (e.g., by a row
  // being added or removed), we need to re-layout the menu in order to resize
  // the view. This may result in redrawing the window. Luckily, this happens
  // only in the case of a row being aded or removed (very rare), and
  // typically happens near menu initialization (rather than once the menu is
  // fully open).
  Layout();
  menu_item_->GetParentMenuItem()->ChildrenChanged();
}

int ExtensionToolbarMenuView::start_padding() const {
  // We pad enough on the left so that the first icon starts at the same point
  // as the labels. We subtract kItemSpacing because there needs to be padding
  // so we can see the drop indicator.
  return views::MenuItemView::label_start() -
      container_->toolbar_actions_bar()->platform_settings().item_spacing;
}

