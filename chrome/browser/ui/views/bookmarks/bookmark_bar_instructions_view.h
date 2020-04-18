// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_BAR_INSTRUCTIONS_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_BAR_INSTRUCTIONS_VIEW_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/link_listener.h"
#include "ui/views/view.h"

class BookmarkBarInstructionsDelegate;

namespace views {
class Label;
class Link;
}

// BookmarkBarInstructionsView is a child of the bookmark bar that is visible
// when the user has no bookmarks on the bookmark bar.
// BookmarkBarInstructionsView shows a description of the bookmarks bar along
// with a link to import bookmarks. Clicking the link results in notifying the
// delegate.
class BookmarkBarInstructionsView : public views::View,
                                    public views::LinkListener,
                                    public views::ContextMenuController {
 public:
  explicit BookmarkBarInstructionsView(
      BookmarkBarInstructionsDelegate* delegate);

 private:
  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;
  const char* GetClassName() const override;
  void OnThemeChanged() override;
  void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // views::LinkListener:
  void LinkClicked(views::Link* source, int event_flags) override;

  // views::ContextMenuController:
  void ShowContextMenuForView(views::View* source,
                              const gfx::Point& point,
                              ui::MenuSourceType source_type) override;

  void UpdateColors();

  BookmarkBarInstructionsDelegate* delegate_;

  views::Label* instructions_;
  views::Link* import_link_;

  // Have the colors of the child views been updated? This is initially false
  // and set to true once we have a valid ThemeProvider.
  bool updated_colors_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkBarInstructionsView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_BAR_INSTRUCTIONS_VIEW_H_
