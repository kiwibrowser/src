// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/chrome_event_processing_window.h"

#include "base/logging.h"
#import "base/mac/foundation_util.h"
#import "chrome/browser/app_controller_mac.h"
#import "chrome/browser/ui/cocoa/chrome_command_dispatcher_delegate.h"
#import "ui/base/cocoa/user_interface_item_command_handler.h"

@implementation ChromeEventProcessingWindow {
 @private
  base::scoped_nsobject<CommandDispatcher> commandDispatcher_;
  base::scoped_nsobject<ChromeCommandDispatcherDelegate>
      commandDispatcherDelegate_;
  base::scoped_nsprotocol<id<UserInterfaceItemCommandHandler>> commandHandler_;
}

- (instancetype)initWithContentRect:(NSRect)contentRect
                          styleMask:(NSUInteger)windowStyle
                            backing:(NSBackingStoreType)bufferingType
                              defer:(BOOL)deferCreation {
  if ((self = [super initWithContentRect:contentRect
                               styleMask:windowStyle
                                 backing:bufferingType
                                   defer:deferCreation])) {
    commandDispatcher_.reset([[CommandDispatcher alloc] initWithOwner:self]);
    commandDispatcherDelegate_.reset(
        [[ChromeCommandDispatcherDelegate alloc] init]);
    [commandDispatcher_ setDelegate:commandDispatcherDelegate_];
  }
  return self;
}

- (BOOL)handleExtraKeyboardShortcut:(NSEvent*)event {
  return [commandDispatcherDelegate_ handleExtraKeyboardShortcut:event
                                                          window:self];
}

// CommandDispatchingWindow implementation.

- (void)setCommandHandler:(id<UserInterfaceItemCommandHandler>)commandHandler {
  commandHandler_.reset([commandHandler retain]);
}

- (BOOL)redispatchKeyEvent:(NSEvent*)event {
  return [commandDispatcher_ redispatchKeyEvent:event];
}

- (BOOL)defaultPerformKeyEquivalent:(NSEvent*)event {
  return [super performKeyEquivalent:event];
}

- (BOOL)defaultValidateUserInterfaceItem:
    (id<NSValidatedUserInterfaceItem>)item {
  return [super validateUserInterfaceItem:item];
}

- (void)commandDispatch:(id)sender {
  [commandDispatcher_ dispatch:sender forHandler:commandHandler_];
}

- (void)commandDispatchUsingKeyModifiers:(id)sender {
  [commandDispatcher_ dispatchUsingKeyModifiers:sender
                                     forHandler:commandHandler_];
}

// NSWindow overrides.

- (BOOL)performKeyEquivalent:(NSEvent*)event {
  return [commandDispatcher_ performKeyEquivalent:event];
}

- (void)sendEvent:(NSEvent*)event {
  if (![commandDispatcher_ preSendEvent:event])
    [super sendEvent:event];
}

// NSWindow overrides (NSUserInterfaceValidations implementation).

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item {
  return [commandDispatcher_ validateUserInterfaceItem:item
                                            forHandler:commandHandler_];
}

@end  // ChromeEventProcessingWindow
