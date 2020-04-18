// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_POPUP_WINDOW_MAC_H_
#define CONTENT_BROWSER_RENDERER_HOST_POPUP_WINDOW_MAC_H_

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "third_party/blink/public/web/web_popup_type.h"
#include "ui/gfx/geometry/rect.h"

@class NSWindow;
@class RenderWidgetHostViewCocoa;

namespace content {

// Helper class for RHWVMacs that are initialized using InitAsPopup. Note that
// this refers to UI that creates its own NSWindow, and does not refer to JS
// initiated popups. This can be tesed using <input type="datetime-local">.
class PopupWindowMac {
 public:
  PopupWindowMac(const gfx::Rect& content_rect,
                 blink::WebPopupType popup_type,
                 RenderWidgetHostViewCocoa* cocoa_view);
  ~PopupWindowMac();

  NSWindow* window() { return popup_window_.get(); }

 private:
  base::scoped_nsobject<NSWindow> popup_window_;

  // Weak.
  RenderWidgetHostViewCocoa* cocoa_view_ = nil;

  DISALLOW_COPY_AND_ASSIGN(PopupWindowMac);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_POPUP_WINDOW_MAC_H_
