// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_BOOKMARK_SUB_MENU_MODEL_H_
#define CHROME_BROWSER_UI_TOOLBAR_BOOKMARK_SUB_MENU_MODEL_H_

// For views and cocoa, we have complex delegate systems to handle
// injecting the bookmarks to the bookmark submenu. This is done to support
// advanced interactions with the menu contents, like right click context menus.

#include "base/macros.h"
#include "ui/base/models/simple_menu_model.h"

class Browser;

class BookmarkSubMenuModel : public ui::SimpleMenuModel {
 public:
  BookmarkSubMenuModel(ui::SimpleMenuModel::Delegate* delegate,
                       Browser* browser);
  ~BookmarkSubMenuModel() override;

 private:
  void Build(Browser* browser);

  DISALLOW_COPY_AND_ASSIGN(BookmarkSubMenuModel);
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_BOOKMARK_SUB_MENU_MODEL_H_
