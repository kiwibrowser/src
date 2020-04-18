// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_CONTEXT_MENU_CLIENT_H_
#define CONTENT_PUBLIC_RENDERER_CONTEXT_MENU_CLIENT_H_

#include "content/common/content_export.h"

namespace content {

// This mirrors the WebKit API in that the "action selected" and "menu closed"
// events are separate. When the user selects and item you'll get both events.
// If the user clicks somewhere else and cancels the menu, you'll just get the
// "closed" event.
class CONTENT_EXPORT ContextMenuClient {
 public:
  // Called when the user selects a context menu item. The request ID will be
  // the value returned by RenderView.ShowCustomContextMenu. This will be
  // followed by a call to OnCustomContextMenuClosed.
  virtual void OnMenuAction(int request_id, unsigned action) = 0;

  // Called when the context menu is closed. The request ID will be the value
  // returned by RenderView.ShowCustomContextMenu.
  virtual void OnMenuClosed(int request_id) = 0;

 protected:
  virtual ~ContextMenuClient() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_CONTEXT_MENU_CLIENT_H_
