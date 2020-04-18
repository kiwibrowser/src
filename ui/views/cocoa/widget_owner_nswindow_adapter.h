// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_COCOA_WIDGET_OWNER_NSWINDOW_ADAPTER_H_
#define UI_VIEWS_COCOA_WIDGET_OWNER_NSWINDOW_ADAPTER_H_

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#import "ui/views/cocoa/bridged_native_widget_owner.h"

@class NSView;
@class NSWindow;
@class WidgetOwnerNSWindowAdapterBridge;

namespace views {

// An adapter that allows a views::Widget to be owned by an NSWindow that is not
// backed by another BridgedNativeWidget.
class WidgetOwnerNSWindowAdapter : public BridgedNativeWidgetOwner {
 public:
  // Create an adapter that will own |child|, tying its lifetime with the
  // NSWindow containing |anchor_view|. The object is self-deleting, via a call
  // to RemoveChildWindow() made in child->OnWindowWillClose().
  WidgetOwnerNSWindowAdapter(BridgedNativeWidget* child, NSView* anchor_view);

  // Called when the owning window is closing.
  void OnWindowWillClose();

  // Called when the owning window is hidden or shown.
  void OnWindowDidChangeOcclusionState();

  // Overridden from BridgedNativeWidgetOwner:
  NSWindow* GetNSWindow() override;
  gfx::Vector2d GetChildWindowOffset() const override;
  bool IsVisibleParent() const override;
  void RemoveChildWindow(BridgedNativeWidget* child) override;

 private:
  // Self-deleting.
  ~WidgetOwnerNSWindowAdapter() override;

  BridgedNativeWidget* child_;  // Weak. Owned by its NativeWidgetMac.
  base::scoped_nsobject<NSView> anchor_view_;
  base::scoped_nsobject<NSWindow> anchor_window_;
  base::scoped_nsobject<WidgetOwnerNSWindowAdapterBridge> observer_bridge_;

  DISALLOW_COPY_AND_ASSIGN(WidgetOwnerNSWindowAdapter);
};

}  // namespace views

#endif  // UI_VIEWS_COCOA_WIDGET_OWNER_NSWINDOW_ADAPTER_H_
