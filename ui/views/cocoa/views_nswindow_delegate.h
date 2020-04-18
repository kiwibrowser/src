// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_COCOA_VIEWS_NSWINDOW_DELEGATE_H_
#define UI_VIEWS_COCOA_VIEWS_NSWINDOW_DELEGATE_H_

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#include "ui/views/views_export.h"

namespace views {
class NativeWidgetMac;
class BridgedNativeWidget;
}

// The delegate set on the NSWindow when a views::BridgedNativeWidget is
// initialized.
VIEWS_EXPORT
@interface ViewsNSWindowDelegate : NSObject<NSWindowDelegate> {
 @private
  views::BridgedNativeWidget* parent_;  // Weak. Owns this.
  base::scoped_nsobject<NSCursor> cursor_;
}

// The NativeWidgetMac that created the window this is attached to. Returns
// NULL if not created by NativeWidgetMac.
@property(nonatomic, readonly) views::NativeWidgetMac* nativeWidgetMac;

// If set, the cursor set in -[NSResponder updateCursor:] when the window is
// reached along the responder chain.
@property(retain, nonatomic) NSCursor* cursor;

// Initialize with the given |parent|.
- (id)initWithBridgedNativeWidget:(views::BridgedNativeWidget*)parent;

// Notify that the window has been reordered in (or removed from) the window
// server's screen list. This is a substitute for -[NSWindowDelegate
// windowDidExpose:], which is only sent for nonretained windows (those without
// a backing store). |notification| is optional and can be set when redirecting
// a notification such as NSApplicationDidHideNotification.
- (void)onWindowOrderChanged:(NSNotification*)notification;

// Notify that the system control tint changed.
- (void)onSystemControlTintChanged:(NSNotification*)notification;

// Called on the delegate of a modal sheet when its modal session ends.
- (void)sheetDidEnd:(NSWindow*)sheet
         returnCode:(NSInteger)returnCode
        contextInfo:(void*)contextInfo;

@end

#endif  // UI_VIEWS_COCOA_VIEWS_NSWINDOW_DELEGATE_H_
