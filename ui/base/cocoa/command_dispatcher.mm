// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/command_dispatcher.h"

#include "base/logging.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/base/cocoa/user_interface_item_command_handler.h"

// Expose -[NSWindow hasKeyAppearance], which determines whether the traffic
// lights on the window are "lit". CommandDispatcher uses this property on a
// parent window to decide whether keys and commands should bubble up.
@interface NSWindow (PrivateAPI)
- (BOOL)hasKeyAppearance;
@end

@interface CommandDispatcher ()
// The parent to bubble events to, or nil.
- (NSWindow<CommandDispatchingWindow>*)bubbleParent;
@end

namespace {

// Duplicate the given key event, but changing the associated window.
NSEvent* KeyEventForWindow(NSWindow* window, NSEvent* event) {
  NSEventType event_type = [event type];

  // Convert the event's location from the original window's coordinates into
  // our own.
  NSPoint location = [event locationInWindow];
  location = ui::ConvertPointFromWindowToScreen([event window], location);
  location = ui::ConvertPointFromScreenToWindow(window, location);

  // Various things *only* apply to key down/up.
  bool is_a_repeat = false;
  NSString* characters = nil;
  NSString* charactors_ignoring_modifiers = nil;
  if (event_type == NSKeyDown || event_type == NSKeyUp) {
    is_a_repeat = [event isARepeat];
    characters = [event characters];
    charactors_ignoring_modifiers = [event charactersIgnoringModifiers];
  }

  // This synthesis may be slightly imperfect: we provide nil for the context,
  // since I (viettrungluu) am sceptical that putting in the original context
  // (if one is given) is valid.
  return [NSEvent keyEventWithType:event_type
                          location:location
                     modifierFlags:[event modifierFlags]
                         timestamp:[event timestamp]
                      windowNumber:[window windowNumber]
                           context:nil
                        characters:characters
       charactersIgnoringModifiers:charactors_ignoring_modifiers
                         isARepeat:is_a_repeat
                           keyCode:[event keyCode]];
}

}  // namespace

@implementation CommandDispatcher {
 @private
  BOOL redispatchingEvent_;
  BOOL eventHandled_;
  NSWindow<CommandDispatchingWindow>* owner_;  // Weak, owns us.
}

@synthesize delegate = delegate_;

- (instancetype)initWithOwner:(NSWindow<CommandDispatchingWindow>*)owner {
  if ((self = [super init])) {
    owner_ = owner;
  }
  return self;
}

- (BOOL)performKeyEquivalent:(NSEvent*)event {
  if ([delegate_ eventHandledByExtensionCommand:event
                                   isRedispatch:redispatchingEvent_]) {
    return YES;
  }

  if (redispatchingEvent_)
    return NO;

  // Give a CommandDispatcherTarget (e.g. a web site) a chance to handle the
  // event. If it doesn't want to handle it, it will call us back with
  // -redispatchKeyEvent:. Only allow this behavior when dispatching key events
  // on the key window.
  if ([owner_ isKeyWindow]) {
    NSResponder* r = [owner_ firstResponder];
    if ([r conformsToProtocol:@protocol(CommandDispatcherTarget)])
      return [r performKeyEquivalent:event];
  }

  if ([delegate_ prePerformKeyEquivalent:event window:owner_])
    return YES;

  if ([owner_ defaultPerformKeyEquivalent:event])
    return YES;

  if ([delegate_ postPerformKeyEquivalent:event window:owner_])
    return YES;

  // Allow commands to "bubble up" to CommandDispatchers in parent windows, if
  // they were not handled here.
  return [[self bubbleParent] performKeyEquivalent:event];
}

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item
                       forHandler:(id<UserInterfaceItemCommandHandler>)handler {
  // Since this class implements these selectors, |super| will always say they
  // are enabled. Only use [super] to validate other selectors. If there is no
  // command handler, defer to AppController.
  if ([item action] == @selector(commandDispatch:) ||
      [item action] == @selector(commandDispatchUsingKeyModifiers:)) {
    if (handler) {
      // -dispatch:.. can't later decide to bubble events because
      // -commandDispatch:.. is assumed to always succeed. So, if there is a
      // |handler|, only validate against that for -commandDispatch:.
      return [handler validateUserInterfaceItem:item window:owner_];
    }

    id appController = [NSApp delegate];
    DCHECK([appController
        conformsToProtocol:@protocol(NSUserInterfaceValidations)]);
    if ([appController validateUserInterfaceItem:item])
      return YES;
  }

  // Note this may validate an action bubbled up from a child window. However,
  // if the child window also -respondsToSelector: (but validated it `NO`), the
  // action will be dispatched to the child only, which may NSBeep().
  // TODO(tapted): Fix this. E.g. bubble up validation via the bubbleParent's
  // CommandDispatcher rather than the NSUserInterfaceValidations protocol, so
  // that this step can be skipped.
  if ([owner_ defaultValidateUserInterfaceItem:item])
    return YES;

  return [[self bubbleParent] validateUserInterfaceItem:item];
}

- (BOOL)redispatchKeyEvent:(NSEvent*)event {
  DCHECK(event);
  NSEventType eventType = [event type];
  if (eventType != NSKeyDown && eventType != NSKeyUp &&
      eventType != NSFlagsChanged) {
    NOTREACHED();
    return YES;  // Pretend it's been handled in an effort to limit damage.
  }

  // Ordinarily, the event's window should be |owner_|. However, when switching
  // between normal and fullscreen mode, we switch out the window, and the
  // event's window might be the previous window (or even an earlier one if the
  // renderer is running slowly and several mode switches occur). In this rare
  // case, we synthesize a new key event so that its associate window (number)
  // is our |owner_|'s.
  if ([event window] != owner_)
    event = KeyEventForWindow(owner_, event);

  // Redispatch the event.
  eventHandled_ = YES;
  redispatchingEvent_ = YES;
  [NSApp sendEvent:event];
  redispatchingEvent_ = NO;

  // If the event was not handled by [NSApp sendEvent:], the sendEvent:
  // method below will be called, and because |redispatchingEvent_| is YES,
  // |eventHandled_| will be set to NO.
  return eventHandled_;
}

- (BOOL)preSendEvent:(NSEvent*)event {
  if (redispatchingEvent_) {
    // If we get here, then the event was not handled by NSApplication.
    eventHandled_ = NO;
    // Return YES to stop native -sendEvent handling.
    return YES;
  }

  return NO;
}

- (void)dispatch:(id)sender
      forHandler:(id<UserInterfaceItemCommandHandler>)handler {
  if (handler)
    [handler commandDispatch:sender window:owner_];
  else
    [[self bubbleParent] commandDispatch:sender];
}

- (void)dispatchUsingKeyModifiers:(id)sender
                       forHandler:(id<UserInterfaceItemCommandHandler>)handler {
  if (handler)
    [handler commandDispatchUsingKeyModifiers:sender window:owner_];
  else
    [[self bubbleParent] commandDispatchUsingKeyModifiers:sender];
}

- (NSWindow<CommandDispatchingWindow>*)bubbleParent {
  NSWindow* parent = [owner_ parentWindow];
  if (parent && [parent hasKeyAppearance] &&
      [parent conformsToProtocol:@protocol(CommandDispatchingWindow)])
    return static_cast<NSWindow<CommandDispatchingWindow>*>(parent);
  return nil;
}

@end
