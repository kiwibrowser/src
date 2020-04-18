// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_BROWSERTEST_UTIL_H_
#define CHROME_BROWSER_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_BROWSERTEST_UTIL_H_

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string16.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"
#include "content/public/common/context_menu_params.h"

class RenderViewContextMenu;

class ContextMenuNotificationObserver {
 public:
  // Wait for a context menu to be shown, and then execute |command_to_execute|.
  explicit ContextMenuNotificationObserver(int command_to_execute);
  ~ContextMenuNotificationObserver();

 private:
  void MenuShown(RenderViewContextMenu* context_menu);

  void ExecuteCommand(RenderViewContextMenu* context_menu);

  int command_to_execute_;

  DISALLOW_COPY_AND_ASSIGN(ContextMenuNotificationObserver);
};

class ContextMenuWaiter {
 public:
  ContextMenuWaiter();
  ~ContextMenuWaiter();

  content::ContextMenuParams& params();

  // Wait until the context menu is opened and closed.
  void WaitForMenuOpenAndClose();

 private:
  void MenuShown(RenderViewContextMenu* context_menu);

  void Cancel(RenderViewContextMenu* context_menu);

  content::ContextMenuParams params_;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(ContextMenuWaiter);
};

#endif  // CHROME_BROWSER_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_BROWSERTEST_UTIL_H_
