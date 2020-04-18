// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/chrome_command_dispatcher_delegate.h"

#include "base/logging.h"
#include "chrome/browser/global_keyboard_shortcuts_mac.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#import "chrome/browser/ui/cocoa/browser_window_controller_private.h"
#import "chrome/browser/ui/cocoa/browser_window_views_mac.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_controller.h"

namespace {

// Type of functions listed in global_keyboard_shortcuts_mac.h.
typedef int (*KeyToCommandMapper)(bool, bool, bool, bool, int, unichar);

// Returns the command that would be executed if |window| received |event|
// according to |command_for_keyboard_shortcut|, or -1 if no command would be
// executed.
int CommandForExtraKeyboardShortcut(
    NSEvent* event,
    NSWindow* window,
    KeyToCommandMapper command_for_keyboard_shortcut) {
  // Extract info from |event|.
  NSUInteger modifers = [event modifierFlags];
  const bool command = modifers & NSCommandKeyMask;
  const bool shift = modifers & NSShiftKeyMask;
  const bool control = modifers & NSControlKeyMask;
  const bool option = modifers & NSAlternateKeyMask;
  const int key_code = [event keyCode];
  const unichar key_char = KeyCharacterForEvent(event);

  int cmd = command_for_keyboard_shortcut(command, shift, control, option,
                                          key_code, key_char);

  // Non-browser windows don't execute any commands.
  if (!chrome::FindBrowserWithWindow(window))
    return -1;

  return cmd;
}

// If the event is for a Browser window, and the key combination has an
// associated command, execute the command.
bool HandleExtraKeyboardShortcut(
    NSEvent* event,
    NSWindow* window,
    KeyToCommandMapper command_for_keyboard_shortcut) {
  int cmd = CommandForExtraKeyboardShortcut(event, window,
                                            command_for_keyboard_shortcut);
  if (cmd == -1)
    return false;

  chrome::ExecuteCommand(chrome::FindBrowserWithWindow(window), cmd);
  return true;
}

bool HandleExtraWindowKeyboardShortcut(NSEvent* event, NSWindow* window) {
  return HandleExtraKeyboardShortcut(event, window,
                                     CommandForWindowKeyboardShortcut);
}

bool HandleDelayedWindowKeyboardShortcut(NSEvent* event, NSWindow* window) {
  return HandleExtraKeyboardShortcut(event, window,
                                     CommandForDelayedWindowKeyboardShortcut);
}

bool HandleExtraBrowserKeyboardShortcut(NSEvent* event, NSWindow* window) {
  return HandleExtraKeyboardShortcut(event, window,
                                     CommandForBrowserKeyboardShortcut);
}

}  // namespace

@implementation ChromeCommandDispatcherDelegate

- (BOOL)handleExtraKeyboardShortcut:(NSEvent*)event window:(NSWindow*)window {
  return HandleExtraBrowserKeyboardShortcut(event, window) ||
         HandleExtraWindowKeyboardShortcut(event, window) ||
         HandleDelayedWindowKeyboardShortcut(event, window);
}

- (BOOL)eventHandledByExtensionCommand:(NSEvent*)event
                          isRedispatch:(BOOL)isRedispatch {
  // Some extension commands have higher priority than web content, and some
  // have lower priority. Regardless of whether the event is being redispatched,
  // let the extension system try to handle the event. In case this is a
  // redispatched event, [event window] gives the correct window.
  if ([event window]) {
    BrowserWindowController* controller =
        BrowserWindowControllerForWindow([event window]);
    // |controller| is only set in Cocoa. In toolkit-views extension commands
    // are handled by BrowserView.
    if ([controller respondsToSelector:@selector(handledByExtensionCommand:
                                                                  priority:)]) {
      ui::AcceleratorManager::HandlerPriority priority =
          isRedispatch ? ui::AcceleratorManager::kNormalPriority
                       : ui::AcceleratorManager::kHighPriority;
      if ([controller handledByExtensionCommand:event priority:priority])
        return YES;
    }
  }
  return NO;
}

- (BOOL)prePerformKeyEquivalent:(NSEvent*)event window:(NSWindow*)window {
  // If a command has a menu key equivalent that *replaces* one of the window
  // keyboard shortcuts, the menu key equivalent needs to be executed, because
  // these are user-addded keyboard shortcuts that replace builtin shortcuts.
  //
  // If a command has a menu key equivalent that does *not* replace a window
  // keyboard shortcut, it will be handled later; only window shortcuts need
  // special handling here since they happen before normal command dispatch.
  int cmd = MenuCommandForKeyEvent(event);
  if (cmd != -1) {
    int keyCmd = CommandForExtraKeyboardShortcut(
        event, window, CommandForWindowKeyboardShortcut);
    Browser* browser = chrome::FindBrowserWithWindow(window);
    if (keyCmd != -1 && browser) {
      chrome::ExecuteCommand(browser, cmd);
      return YES;
    }
  }

  // Handle per-window shortcuts like cmd-1, but do not handle browser-level
  // shortcuts like cmd-left (else, cmd-left would do history navigation even
  // if e.g. the Omnibox has focus).
  return HandleExtraWindowKeyboardShortcut(event, window);
}

- (BOOL)postPerformKeyEquivalent:(NSEvent*)event window:(NSWindow*)window {
  // Handle per-window shortcuts like Esc after giving everybody else a chance
  // to handle them
  return HandleDelayedWindowKeyboardShortcut(event, window);
}

@end  // ChromeCommandDispatchDelegate
