// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_MAC_H_
#define CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_MAC_H_

#import <Cocoa/Cocoa.h>

#include <memory>
#include <string>
#include <vector>

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "content/browser/frame_host/popup_menu_helper_mac.h"
#include "content/browser/renderer_host/render_view_host_delegate_view.h"
#include "content/browser/web_contents/web_contents_view.h"
#include "content/common/content_export.h"
#include "content/common/drag_event_source_info.h"
#include "ui/base/cocoa/base_view.h"
#include "ui/gfx/geometry/size.h"

@class WebDragDest;
@class WebDragSource;

namespace content {
class RenderWidgetHostViewMac;
class WebContentsImpl;
class WebContentsViewDelegate;
class WebContentsViewMac;
}

namespace gfx {
class Vector2d;
}

namespace ui {
class Layer;
}

CONTENT_EXPORT
@interface WebContentsViewCocoa : BaseView {
 @private
  // Instances of this class are owned by both webContentsView_ and AppKit. It
  // is possible for an instance to outlive its webContentsView_. The
  // webContentsView_ must call -clearWebContentsView in its destructor.
  content::WebContentsViewMac* webContentsView_;
  base::scoped_nsobject<WebDragSource> dragSource_;
  base::scoped_nsobject<WebDragDest> dragDest_;
  BOOL mouseDownCanMoveWindow_;
}

- (void)setMouseDownCanMoveWindow:(BOOL)canMove;

// Returns the available drag operations. This is a required method for
// NSDraggingSource. It is supposedly deprecated, but the non-deprecated API
// -[NSWindow dragImage:...] still relies on it.
- (NSDragOperation)draggingSourceOperationMaskForLocal:(BOOL)isLocal;
@end

namespace content {

// Mac-specific implementation of the WebContentsView. It owns an NSView that
// contains all of the contents of the tab and associated child views.
class WebContentsViewMac : public WebContentsView,
                           public RenderViewHostDelegateView,
                           public PopupMenuHelper::Delegate {
 public:
  // The corresponding WebContentsImpl is passed in the constructor, and manages
  // our lifetime. This doesn't need to be the case, but is this way currently
  // because that's what was easiest when they were split.
  WebContentsViewMac(WebContentsImpl* web_contents,
                     WebContentsViewDelegate* delegate);
  ~WebContentsViewMac() override;

  // WebContentsView implementation --------------------------------------------
  gfx::NativeView GetNativeView() const override;
  gfx::NativeView GetContentNativeView() const override;
  gfx::NativeWindow GetTopLevelNativeWindow() const override;
  void GetContainerBounds(gfx::Rect* out) const override;
  void SizeContents(const gfx::Size& size) override;
  void Focus() override;
  void SetInitialFocus() override;
  void StoreFocus() override;
  void RestoreFocus() override;
  void FocusThroughTabTraversal(bool reverse) override;
  DropData* GetDropData() const override;
  gfx::Rect GetViewBounds() const override;
  void SetAllowOtherViews(bool allow) override;
  bool GetAllowOtherViews() const override;
  void CreateView(const gfx::Size& initial_size,
                  gfx::NativeView context) override;
  RenderWidgetHostViewBase* CreateViewForWidget(
      RenderWidgetHost* render_widget_host,
      bool is_guest_view_hack) override;
  RenderWidgetHostViewBase* CreateViewForPopupWidget(
      RenderWidgetHost* render_widget_host) override;
  void SetPageTitle(const base::string16& title) override;
  void RenderViewCreated(RenderViewHost* host) override;
  void RenderViewSwappedIn(RenderViewHost* host) override;
  void SetOverscrollControllerEnabled(bool enabled) override;
  bool IsEventTracking() const override;
  void CloseTabAfterEventTracking() override;

  // RenderViewHostDelegateView:
  void StartDragging(const DropData& drop_data,
                     blink::WebDragOperationsMask allowed_operations,
                     const gfx::ImageSkia& image,
                     const gfx::Vector2d& image_offset,
                     const DragEventSourceInfo& event_info,
                     RenderWidgetHostImpl* source_rwh) override;
  void UpdateDragCursor(blink::WebDragOperation operation) override;
  void GotFocus(RenderWidgetHostImpl* render_widget_host) override;
  void TakeFocus(bool reverse) override;
  void ShowContextMenu(RenderFrameHost* render_frame_host,
                       const ContextMenuParams& params) override;
  void ShowPopupMenu(RenderFrameHost* render_frame_host,
                     const gfx::Rect& bounds,
                     int item_height,
                     double item_font_size,
                     int selected_item,
                     const std::vector<MenuItem>& items,
                     bool right_aligned,
                     bool allow_multiple_selection) override;
  void HidePopupMenu() override;

  // PopupMenuHelper::Delegate:
  void OnMenuClosed() override;

  // A helper method for closing the tab in the
  // CloseTabAfterEventTracking() implementation.
  void CloseTab();

  void SetParentUiLayer(ui::Layer* parent_ui_layer);

  WebContentsImpl* web_contents() { return web_contents_; }
  WebContentsViewDelegate* delegate() { return delegate_.get(); }

  using RenderWidgetHostViewCreateFunction =
      RenderWidgetHostViewMac* (*)(RenderWidgetHost*, bool);

  // Used to override the creation of RenderWidgetHostViews in tests.
  CONTENT_EXPORT static void InstallCreateHookForTests(
      RenderWidgetHostViewCreateFunction create_render_widget_host_view);

 private:
  // Returns the fullscreen view, if one exists; otherwise, returns the content
  // native view. This ensures that the view currently attached to a NSWindow is
  // being used to query or set first responder state.
  gfx::NativeView GetNativeViewForFocus() const;

  // The WebContentsImpl whose contents we display.
  WebContentsImpl* web_contents_;

  // The Cocoa NSView that lives in the view hierarchy.
  base::scoped_nsobject<WebContentsViewCocoa> cocoa_view_;

  // Our optional delegate.
  std::unique_ptr<WebContentsViewDelegate> delegate_;

  // Whether to allow other views.
  bool allow_other_views_;

  ui::Layer* parent_ui_layer_ = nullptr;

  std::unique_ptr<PopupMenuHelper> popup_menu_helper_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsViewMac);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_MAC_H_
