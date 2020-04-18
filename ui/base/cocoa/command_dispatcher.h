// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_COMMAND_DISPATCHER_H_
#define UI_BASE_COCOA_COMMAND_DISPATCHER_H_

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#include "ui/base/ui_base_export.h"

@protocol CommandDispatcherDelegate;
@protocol CommandDispatchingWindow;
@protocol UserInterfaceItemCommandHandler;

// CommandDispatcher guides the processing of key events to ensure key commands
// are executed in the appropriate order. In particular, it allows a first
// responder implementing CommandDispatcherTarget to handle an event
// asynchronously and return unhandled events via -redispatchKeyEvent:. An
// NSWindow can use CommandDispatcher by implementing CommandDispatchingWindow
// and overriding -[NSWindow performKeyEquivalent:] and -[NSWindow sendEvent:]
// to call the respective CommandDispatcher methods.
UI_BASE_EXPORT @interface CommandDispatcher : NSObject

@property(assign, nonatomic) id<CommandDispatcherDelegate> delegate;

- (instancetype)initWithOwner:(NSWindow<CommandDispatchingWindow>*)owner;

// The main entry point for key events. The CommandDispatchingWindow should
// override -[NSResponder performKeyEquivalent:] and call this instead. Returns
// YES if the event is handled.
- (BOOL)performKeyEquivalent:(NSEvent*)event;

// Validate a user interface item (e.g. an NSMenuItem), consulting |handler| for
// -commandDispatch: item actions.
- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item
                       forHandler:(id<UserInterfaceItemCommandHandler>)handler;

// Sends a key event to -[NSApp sendEvent:]. This is used to allow default
// AppKit handling of an event that comes back from CommandDispatcherTarget,
// e.g. key equivalents in the menu, or window manager commands like Cmd+`. Once
// the event returns to the window at -preSendEvent:, handling will stop. The
// event must be of type |NSKeyDown|, |NSKeyUp|, or |NSFlagsChanged|. Returns
// YES if the event is handled.
- (BOOL)redispatchKeyEvent:(NSEvent*)event;

// The CommandDispatchingWindow should override -[NSWindow sendEvent:] and call
// this before a native -sendEvent:. Ensures that a redispatched event is not
// reposted infinitely. Returns YES if the event is handled.
- (BOOL)preSendEvent:(NSEvent*)event;

// Dispatch a -commandDispatch: action either to |handler| or a parent window's
// handler.
- (void)dispatch:(id)sender
      forHandler:(id<UserInterfaceItemCommandHandler>)handler;
- (void)dispatchUsingKeyModifiers:(id)sender
                       forHandler:(id<UserInterfaceItemCommandHandler>)handler;

@end

// If the NSWindow's firstResponder implements CommandDispatcherTarget, it is
// given the first opportunity to process a command.
@protocol CommandDispatcherTarget

// To handle an event asynchronously, return YES. If the event is ultimately not
// handled, return the event to the CommandDispatchingWindow via -[[event
// window] redispatchKeyEvent:event].
- (BOOL)performKeyEquivalent:(NSEvent*)event;

@end

// Provides CommandDispatcher with the means to redirect key equivalents at
// different stages of event handling.
@protocol CommandDispatcherDelegate<NSObject>

// Called before any other event handling, and possibly again if an unhandled
// event comes back from CommandDispatcherTarget.
- (BOOL)eventHandledByExtensionCommand:(NSEvent*)event
                          isRedispatch:(BOOL)isRedispatch;

// Called before the default -performKeyEquivalent:, but after the
// CommandDispatcherTarget has had a chance to intercept it. |window| is the
// CommandDispatchingWindow that owns CommandDispatcher.
- (BOOL)prePerformKeyEquivalent:(NSEvent*)event window:(NSWindow*)window;

// Called after the default -performKeyEquivalent:. |window| is the
// CommandDispatchingWindow that owns CommandDispatcher.
- (BOOL)postPerformKeyEquivalent:(NSEvent*)event window:(NSWindow*)window;

@end

// The set of methods an NSWindow subclass needs to implement to use
// CommandDispatcher.
@protocol CommandDispatchingWindow

// If set, NSUserInterfaceItemValidations for -commandDispatch: and
// -commandDispatchUsingKeyModifiers: will be redirected to the command handler.
// Retains |commandHandler|.
-(void)setCommandHandler:(id<UserInterfaceItemCommandHandler>) commandHandler;

// This can be implemented with -[CommandDispatcher redispatchKeyEvent:]. It's
// so that callers can simply return events to the NSWindow.
- (BOOL)redispatchKeyEvent:(NSEvent*)event;

// Short-circuit to the default -[NSResponder performKeyEquivalent:] which
// CommandDispatcher calls as part of its -performKeyEquivalent: flow.
- (BOOL)defaultPerformKeyEquivalent:(NSEvent*)event;

// Short-circuit to the default -validateUserInterfaceItem: implementation.
- (BOOL)defaultValidateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item;

// AppKit will call -[NSUserInterfaceValidations validateUserInterfaceItem:] to
// validate UI items. Any item whose target is FirstResponder, or nil, will
// traverse the responder chain looking for a responder that implements the
// item's selector. Thus NSWindow is usually the last to be checked and will
// handle any items that are not validated elsewhere in the chain. Implement the
// following so that menu items with these selectors are validated by
// CommandDispatchingWindow.
- (void)commandDispatch:(id)sender;
- (void)commandDispatchUsingKeyModifiers:(id)sender;

@end

#endif  // UI_BASE_COCOA_COMMAND_DISPATCHER_H_
