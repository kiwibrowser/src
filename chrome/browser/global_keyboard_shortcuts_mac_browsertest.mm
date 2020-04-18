// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/global_keyboard_shortcuts_mac.h"

#import <Cocoa/Cocoa.h>

#include "base/run_loop.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views_mode_controller.h"
#import "ui/events/test/cocoa_test_event_utils.h"

using cocoa_test_event_utils::SynthesizeKeyEvent;

using GlobalKeyboardShortcutsTest = extensions::ExtensionBrowserTest;

namespace {

void ActivateAccelerator(NSWindow* window, NSEvent* ns_event) {
  if ([window performKeyEquivalent:ns_event])
    return;

  // This is consistent with the way AppKit dispatches events when
  // -performKeyEquivalent: returns NO. See "The Path of Key Events" in the
  // Cocoa Event Architecture documentation.
  [window sendEvent:ns_event];
}

}  // namespace

// Test that global keyboard shortcuts are handled by the native window.
IN_PROC_BROWSER_TEST_F(GlobalKeyboardShortcutsTest, SwitchTabsMac) {
  NSWindow* ns_window = browser()->window()->GetNativeWindow();
  TabStripModel* tab_strip = browser()->tab_strip_model();

  // Set up window with 2 tabs.
  chrome::NewTab(browser());
  EXPECT_EQ(2, tab_strip->count());
  EXPECT_TRUE(tab_strip->IsTabSelected(1));

  // Ctrl+Tab goes to the next tab, which loops back to the first tab.
  ActivateAccelerator(
      ns_window,
      SynthesizeKeyEvent(ns_window, true, ui::VKEY_TAB, NSControlKeyMask));
  EXPECT_TRUE(tab_strip->IsTabSelected(0));

  // Cmd+2 goes to the second tab.
  ActivateAccelerator(ns_window, SynthesizeKeyEvent(ns_window, true, ui::VKEY_2,
                                                    NSCommandKeyMask));
  EXPECT_TRUE(tab_strip->IsTabSelected(1));

  // Cmd+{ goes to the previous tab.
  ActivateAccelerator(ns_window,
                      SynthesizeKeyEvent(ns_window, true, ui::VKEY_OEM_4,
                                         NSShiftKeyMask | NSCommandKeyMask));
  EXPECT_TRUE(tab_strip->IsTabSelected(0));
}

IN_PROC_BROWSER_TEST_F(GlobalKeyboardShortcutsTest, MenuCommandPriority) {
  // This test doesn't work in Views mode at the moment:
  // https://crbug.com/845503. Disabled pending a bit of design work to fix it.
  if (!views_mode_controller::IsViewsBrowserCocoa())
    return;
  NSWindow* ns_window = browser()->window()->GetNativeWindow();
  TabStripModel* tab_strip = browser()->tab_strip_model();

  // Set up window with 4 tabs.
  chrome::NewTab(browser());
  chrome::NewTab(browser());
  chrome::NewTab(browser());
  EXPECT_EQ(4, tab_strip->count());
  EXPECT_TRUE(tab_strip->IsTabSelected(3));

  // Use the cmd-2 hotkey to switch to the second tab.
  ActivateAccelerator(ns_window, SynthesizeKeyEvent(ns_window, true, ui::VKEY_2,
                                                    NSCommandKeyMask));
  EXPECT_TRUE(tab_strip->IsTabSelected(1));

  // Change the "Select Next Tab" menu item's key equivalent to be cmd-2, to
  // simulate what would happen if there was a user key equivalent for it. Note
  // that there is a readonly "userKeyEquivalent" property on NSMenuItem, but
  // this code can't modify it.
  NSMenu* main_menu = [NSApp mainMenu];
  ASSERT_NE(nil, main_menu);
  NSMenuItem* window_menu = [main_menu itemWithTitle:@"Window"];
  ASSERT_NE(nil, window_menu);
  ASSERT_TRUE(window_menu.hasSubmenu);
  NSMenuItem* next_item = [window_menu.submenu itemWithTag:IDC_SELECT_NEXT_TAB];
  ASSERT_NE(nil, next_item);
  [next_item setKeyEquivalent:@"2"];
  [next_item setKeyEquivalentModifierMask:NSCommandKeyMask];

  // Send cmd-2 again, and ensure the tab switches.
  ActivateAccelerator(ns_window, SynthesizeKeyEvent(ns_window, true, ui::VKEY_2,
                                                    NSCommandKeyMask));
  EXPECT_TRUE(tab_strip->IsTabSelected(2));
  ActivateAccelerator(ns_window, SynthesizeKeyEvent(ns_window, true, ui::VKEY_2,
                                                    NSCommandKeyMask));
  EXPECT_TRUE(tab_strip->IsTabSelected(3));
}
