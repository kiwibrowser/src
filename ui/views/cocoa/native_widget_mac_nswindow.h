// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_COCOA_NATIVE_WIDGET_MAC_NSWINDOW_H_
#define UI_VIEWS_COCOA_NATIVE_WIDGET_MAC_NSWINDOW_H_

#import <Cocoa/Cocoa.h>

#import "ui/base/cocoa/command_dispatcher.h"
#include "ui/views/views_export.h"

// The NSWindow used by BridgedNativeWidget. Provides hooks into AppKit that
// can only be accomplished by overriding methods.
VIEWS_EXPORT
@interface NativeWidgetMacNSWindow : NSWindow<CommandDispatchingWindow>

// Set a CommandDispatcherDelegate, i.e. to implement key event handling.
- (void)setCommandDispatcherDelegate:(id<CommandDispatcherDelegate>)delegate;

@end

#endif  // UI_VIEWS_COCOA_NATIVE_WIDGET_MAC_NSWINDOW_H_
