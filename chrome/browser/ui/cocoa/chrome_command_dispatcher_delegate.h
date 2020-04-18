// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CHROME_COMMAND_DISPATCHER_DELEGATE_H_
#define CHROME_BROWSER_UI_COCOA_CHROME_COMMAND_DISPATCHER_DELEGATE_H_

#import <Cocoa/Cocoa.h>

#import "ui/base/cocoa/command_dispatcher.h"

// Implement CommandDispatcherDelegate by intercepting browser window keyboard
// shortcuts and executing them with chrome::ExecuteCommand.
@interface ChromeCommandDispatcherDelegate : NSObject<CommandDispatcherDelegate>

// Checks if |event| is a keyboard shortcut listed in
// global_keyboard_shortcuts_mac.h. If so, execute the associated command.
// Returns YES if the event was handled.
- (BOOL)handleExtraKeyboardShortcut:(NSEvent*)event window:(NSWindow*)window;

@end

#endif  // CHROME_BROWSER_UI_COCOA_CHROME_COMMAND_DISPATCHER_DELEGATE_H_
