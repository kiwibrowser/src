// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GLOBAL_KEYBOARD_SHORTCUTS_MAC_H_
#define CHROME_BROWSER_GLOBAL_KEYBOARD_SHORTCUTS_MAC_H_

#include <Carbon/Carbon.h>  // For unichar.
#include <stddef.h>

#include <vector>

@class NSEvent;

struct KeyboardShortcutData {
  bool command_key;
  bool shift_key;
  bool cntrl_key;
  bool opt_key;
  // Either one of vkey_code or key_char must be specified.  For keys
  // whose virtual key code is hardware-dependent (kVK_ANSI_*) key_char
  // should be specified instead.
  // Set 0 for the one you do not want to specify.
  int vkey_code;  // Virtual Key code for the command.
  unichar key_char;  // Key event characters for the command as reported by
                     // [NSEvent charactersIgnoringModifiers].
  int chrome_command;  // The chrome command # to execute for this shortcut.
};

// Check if a given keycode + modifiers (or keychar + modifiers if the
// |key_char| is specified) correspond to a given Chrome command.
// returns: Command number (as passed to
// BrowserCommandController::ExecuteCommand) or -1 if there was no match.
//
// |performKeyEquivalent:| bubbles events up from the window to the views.  If
// we let it bubble up to the Omnibox, then the Omnibox handles cmd-left/right
// just fine, but it swallows cmd-1 and doesn't give us a chance to intercept
// this. Hence, we need three types of keyboard shortcuts: shortcuts that are
// intercepted before the Omnibox handles events, shortcuts that are
// intercepted after the Omnibox had a chance but did not handle them, and
// shortcuts that are only handled when tab contents is focused.
//
// This means cmd-left doesn't work if you hit cmd-l tab, which focusses
// something that's neither omnibox nor tab contents. This behavior is
// consistent with safari and camino, and I think it's the best we can do
// without rewriting event dispatching ( http://crbug.com/251069 ).

// This returns shortcuts that should work no matter what component of the
// browser is focused. They are executed by the window, before any view has the
// opportunity to override the shortcut (with the exception of the tab contents,
// which first checks if the current web page wants to handle the shortcut).
int CommandForWindowKeyboardShortcut(
    bool command_key, bool shift_key, bool cntrl_key, bool opt_key,
    int vkey_code, unichar key_char);

// This returns shortcuts that should work no matter what component of the
// browser is focused. They are executed by the window, after any view has the
// opportunity to override the shortcut
int CommandForDelayedWindowKeyboardShortcut(
    bool command_key, bool shift_key, bool cntrl_key, bool opt_key,
    int vkey_code, unichar key_char);

// This returns shortcuts that should work only if the tab contents have focus
// (e.g. cmd-left, which shouldn't do history navigation if e.g. the omnibox has
// focus).
int CommandForBrowserKeyboardShortcut(
    bool command_key, bool shift_key, bool cntrl_key, bool opt_key,
    int vkey_code, unichar key_char);

// Returns the Chrome command associated with |event|, or -1 if not found.
int CommandForKeyEvent(NSEvent* event);

// Returns the menu command associated with |event|, or -1 if not found.
int MenuCommandForKeyEvent(NSEvent* event);

// Returns a keyboard event character for the given |event|.  In most cases
// this returns the first character of [NSEvent charactersIgnoringModifiers],
// but when [NSEvent character] has different printable ascii character
// we may return the first character of [NSEvent characters] instead.
// (E.g. for dvorak-qwerty layout we want [NSEvent characters] rather than
// [charactersIgnoringModifiers] for command keys.  Similarly, on german
// layout we want '{' character rather than '8' for opt-8.)
unichar KeyCharacterForEvent(NSEvent* event);

// For testing purposes.
const std::vector<KeyboardShortcutData>& GetWindowKeyboardShortcutTable();
const std::vector<KeyboardShortcutData>&
GetDelayedWindowKeyboardShortcutTable();
const std::vector<KeyboardShortcutData>& GetBrowserKeyboardShortcutTable();

#endif  // #ifndef CHROME_BROWSER_GLOBAL_KEYBOARD_SHORTCUTS_MAC_H_
