// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_COCOA_BRIDGED_NATIVE_WIDGET_OWNER_H_
#define UI_VIEWS_COCOA_BRIDGED_NATIVE_WIDGET_OWNER_H_

namespace gfx {
class Vector2d;
}

@class NSWindow;

namespace views {

class BridgedNativeWidget;

// An abstract interface wrapping an NSWindow that ties the lifetime of one or
// more child BridgedNativeWidgets to the lifetime of that NSWindow. This is not
// simply an NSWindow, because the child window API provided by NSWindow
// requires child windows to always be visible.
class BridgedNativeWidgetOwner {
 public:
  // The NSWindow parent.
  virtual NSWindow* GetNSWindow() = 0;

  // The offset in screen pixels for positioning child windows owned by |this|.
  virtual gfx::Vector2d GetChildWindowOffset() const = 0;

  // Return false if |this| is hidden, or has a hidden ancestor.
  virtual bool IsVisibleParent() const = 0;

  // Removes a child window. Note |this| may be deleted after calling, so the
  // caller should immediately null out the pointer used to make the call.
  virtual void RemoveChildWindow(BridgedNativeWidget* child) = 0;

 protected:
  // Instances of this class may be self-deleting.
  virtual ~BridgedNativeWidgetOwner() {}
};

}  // namespace views

#endif  // UI_VIEWS_COCOA_BRIDGED_NATIVE_WIDGET_OWNER_H_
