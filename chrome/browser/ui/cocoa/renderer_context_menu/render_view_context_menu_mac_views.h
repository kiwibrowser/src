// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_MAC_VIEWS_H_
#define CHROME_BROWSER_UI_COCOA_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_MAC_VIEWS_H_

#import <Cocoa/Cocoa.h>

#include "chrome/browser/ui/cocoa/renderer_context_menu/render_view_context_menu_mac.h"

// Mac Views implementation of the renderer context menu display code.
class RenderViewContextMenuMacViews : public RenderViewContextMenuMac {
 public:
  RenderViewContextMenuMacViews(content::RenderFrameHost* render_frame_host,
                                const content::ContextMenuParams& params,
                                NSView* parent_view);

  ~RenderViewContextMenuMacViews() override;

  // RenderViewContextMenuMac:
  void Show() override;

 private:
  friend class ToolkitDelegateViewsMac;

  NSView* parent_view_;

  DISALLOW_COPY_AND_ASSIGN(RenderViewContextMenuMacViews);
};

#endif  // CHROME_BROWSER_UI_COCOA_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_MAC_VIEWS_H_