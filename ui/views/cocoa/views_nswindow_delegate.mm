// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/views/cocoa/views_nswindow_delegate.h"

#include "base/logging.h"
#import "base/mac/bind_objc_block.h"
#include "base/threading/thread_task_runner_handle.h"
#import "ui/views/cocoa/bridged_content_view.h"
#import "ui/views/cocoa/bridged_native_widget.h"
#include "ui/views/widget/native_widget_mac.h"

@implementation ViewsNSWindowDelegate

- (id)initWithBridgedNativeWidget:(views::BridgedNativeWidget*)parent {
  DCHECK(parent);
  if ((self = [super init])) {
    parent_ = parent;
  }
  return self;
}

- (views::NativeWidgetMac*)nativeWidgetMac {
  return parent_->native_widget_mac();
}

- (NSCursor*)cursor {
  return cursor_.get();
}

- (void)setCursor:(NSCursor*)newCursor {
  if (cursor_.get() == newCursor)
    return;

  cursor_.reset([newCursor retain]);
  [parent_->ns_window() resetCursorRects];
}

- (void)onWindowOrderChanged:(NSNotification*)notification {
  parent_->OnVisibilityChanged();
}

- (void)onSystemControlTintChanged:(NSNotification*)notification {
  parent_->OnSystemControlTintChanged();
}

- (void)sheetDidEnd:(NSWindow*)sheet
         returnCode:(NSInteger)returnCode
        contextInfo:(void*)contextInfo {
  // |parent_| will be null when triggered from the block in -windowWillClose:.
  if (!parent_)
    return;

  [sheet orderOut:nil];
  parent_->OnWindowWillClose();
}

// NSWindowDelegate implementation.

- (void)windowDidFailToEnterFullScreen:(NSWindow*)window {
  // Cocoa should already have sent an (unexpected) windowDidExitFullScreen:
  // notification, and the attempt to get back into fullscreen should fail.
  // Nothing to do except verify |parent_| is no longer trying to fullscreen.
  DCHECK(!parent_->target_fullscreen_state());
}

- (void)windowDidFailToExitFullScreen:(NSWindow*)window {
  // Unlike entering fullscreen, windowDidFailToExitFullScreen: is sent *before*
  // windowDidExitFullScreen:. Also, failing to exit fullscreen just dumps the
  // window out of fullscreen without an animation; still sending the expected,
  // windowDidExitFullScreen: notification. So, again, nothing to do here.
  DCHECK(!parent_->target_fullscreen_state());
}

- (void)windowDidResize:(NSNotification*)notification {
  parent_->OnSizeChanged();
}

- (void)windowDidMove:(NSNotification*)notification {
  // Note: windowDidMove: is sent only once at the end of a window drag. There
  // is also windowWillMove: sent at the start, also once. When the window is
  // being moved by the WindowServer live updates are not provided.
  parent_->OnPositionChanged();
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
  parent_->OnWindowKeyStatusChangedTo(true);
}

- (void)windowDidResignKey:(NSNotification*)notification {
  parent_->OnWindowKeyStatusChangedTo(false);
}

- (BOOL)windowShouldClose:(id)sender {
  views::NonClientView* nonClientView =
      [self nativeWidgetMac]->GetWidget()->non_client_view();
  return !nonClientView || nonClientView->CanClose();
}

- (void)windowWillClose:(NSNotification*)notification {
  // Retain |self|. |parent_| should be cleared. OnWindowWillClose() may delete
  // |parent_|, but it may also dealloc |self| before returning. However, the
  // observers it notifies before that need a valid |parent_| on the delegate,
  // so it can only be cleared after OnWindowWillClose() returns.
  base::scoped_nsobject<NSObject> keepAlive(self, base::scoped_policy::RETAIN);
  NSWindow* window = parent_->ns_window();
  if (NSWindow* sheetParent = [window sheetParent]) {
    // On no! Something called -[NSWindow close] on a sheet rather than calling
    // -[NSWindow endSheet:] on its parent. If the modal session is not ended
    // then the parent will never be able to show another sheet. But calling
    // -endSheet: here will block the thread with an animation, so post a task.
    // Use a block: The argument to -endSheet: must be retained, since it's the
    // window that is closing and -performSelector: won't retain the argument.
    // The NSWindowDelegate (i.e. |self|) must also be explicitly retained. Even
    // though the call to OnWindowWillClose() below will remove |self| as the
    // NSWindow delegate, the call to -[NSApp beginSheet:] also took a weak
    // reference to the delegate, which will be destroyed when the sheet's
    // BridgedNativeWidget is destroyed.
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, base::BindBlock(^{
      [sheetParent endSheet:window];
      [[self retain] release];  // Force |self| to be retained for the block.
    }));
  }
  DCHECK([window isEqual:[notification object]]);
  parent_->OnWindowWillClose();
  parent_ = nullptr;
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
  parent_->OnVisibilityChanged();
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
  parent_->OnVisibilityChanged();
}

- (void)windowDidChangeBackingProperties:(NSNotification*)notification {
  parent_->OnBackingPropertiesChanged();
}

- (void)windowWillEnterFullScreen:(NSNotification*)notification {
  parent_->OnFullscreenTransitionStart(true);
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification {
  parent_->OnFullscreenTransitionComplete(true);
}

- (void)windowWillExitFullScreen:(NSNotification*)notification {
  parent_->OnFullscreenTransitionStart(false);
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
  parent_->OnFullscreenTransitionComplete(false);
}

// Allow non-resizable windows (without NSResizableWindowMask) to fill the
// screen in fullscreen mode. This only happens when
// -[NSWindow toggleFullscreen:] is called since non-resizable windows have no
// fullscreen button. Without this they would only enter fullscreen at their
// current size.
- (NSSize)window:(NSWindow*)window
    willUseFullScreenContentSize:(NSSize)proposedSize {
  return proposedSize;
}

// Override to correctly position modal dialogs.
- (NSRect)window:(NSWindow*)window
    willPositionSheet:(NSWindow*)sheet
            usingRect:(NSRect)defaultSheetLocation {
  // As per NSWindowDelegate documentation, the origin indicates the top left
  // point of the host frame in window coordinates. The width changes the
  // animation from vertical to trapezoid if it is smaller than the width of the
  // dialog. The height is ignored but should be set to zero.
  return NSMakeRect(0, [self nativeWidgetMac]->SheetPositionY(),
                    NSWidth(defaultSheetLocation), 0);
}

@end
