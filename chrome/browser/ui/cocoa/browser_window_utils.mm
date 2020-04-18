// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/browser_window_utils.h"

#include "base/logging.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/global_keyboard_shortcuts_mac.h"
#include "chrome/browser/ui/browser.h"
#import "chrome/browser/ui/cocoa/chrome_event_processing_window.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_controller.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/events/keycodes/keyboard_codes.h"

using content::NativeWebKeyboardEvent;

namespace {

CGFloat GetPatternVerticalOffsetWithTabStrip(bool tabStripVisible) {
  // Without tab strip, offset an extra pixel (determined by experimentation).
  return tabStripVisible ? -1 : 0;
}

}  // namespace


@implementation BrowserWindowUtils
+ (BOOL)shouldHandleKeyboardEvent:(const NativeWebKeyboardEvent&)event {
  if (event.skip_in_browser || event.GetType() == NativeWebKeyboardEvent::kChar)
    return NO;
  DCHECK(event.os_event != NULL);
  return YES;
}

+ (BOOL)isTextEditingEvent:(const content::NativeWebKeyboardEvent&)event {
  return (event.GetModifiers() & blink::WebInputEvent::kMetaKey) &&
         (event.windows_key_code == ui::VKEY_A ||
          event.windows_key_code == ui::VKEY_V ||
          event.windows_key_code == ui::VKEY_C ||
          event.windows_key_code == ui::VKEY_X ||
          event.windows_key_code == ui::VKEY_Z);
}

+ (int)getCommandId:(const NativeWebKeyboardEvent&)event {
  return CommandForKeyEvent(event.os_event);
}

+ (BOOL)handleKeyboardEvent:(NSEvent*)event
                   inWindow:(NSWindow*)window {
  ChromeEventProcessingWindow* event_window =
      static_cast<ChromeEventProcessingWindow*>(window);
  DCHECK([event_window isKindOfClass:[ChromeEventProcessingWindow class]]);

  // Do not fire shortcuts on key up.
  if ([event type] == NSKeyDown) {
    // Send the event to the menu before sending it to the browser/window
    // shortcut handling, so that if a user configures cmd-left to mean
    // "previous tab", it takes precedence over the built-in "history back"
    // binding. Other than that, the |-redispatchKeyEvent:| call would take care
    // of invoking the original menu item shortcut as well.

    if ([[NSApp mainMenu] performKeyEquivalent:event])
      return true;

    if ([event_window handleExtraKeyboardShortcut:event])
      return true;
  }

  return [event_window redispatchKeyEvent:event];
}

+ (NSString*)scheduleReplaceOldTitle:(NSString*)oldTitle
                        withNewTitle:(NSString*)newTitle
                           forWindow:(NSWindow*)window {
  if (oldTitle)
    [[NSRunLoop currentRunLoop]
        cancelPerformSelector:@selector(setTitle:)
                       target:window
                     argument:oldTitle];

  [[NSRunLoop currentRunLoop]
      performSelector:@selector(setTitle:)
               target:window
             argument:newTitle
                order:0
                modes:[NSArray arrayWithObject:NSDefaultRunLoopMode]];
  return [newTitle copy];
}

// The titlebar/tabstrip header on the mac is slightly smaller than on Windows.
// There is also no window frame to the left and right of the web contents on
// mac.
// To keep:
// - the window background pattern (IDR_THEME_FRAME.*) lined up vertically with
// the tab and toolbar patterns
// - the toolbar pattern lined up horizontally with the NTP background.
// we have to shift the pattern slightly, rather than drawing from the top left
// corner of the frame / tabstrip. The offsets below were empirically determined
// in order to line these patterns up.
//
// This will make the themes look slightly different than in Windows/Linux
// because of the differing heights between window top and tab top, but this has
// been approved by UI.
const CGFloat kPatternHorizontalOffset = -5;

+ (NSPoint)themeImagePositionFor:(NSView*)windowView
                    withTabStrip:(NSView*)tabStripView
                       alignment:(ThemeImageAlignment)alignment {
  if (!tabStripView) {
    return NSMakePoint(kPatternHorizontalOffset,
                       NSHeight([windowView bounds]) +
                           GetPatternVerticalOffsetWithTabStrip(false));
  }

  NSPoint position =
      [BrowserWindowUtils themeImagePositionInTabStripCoords:tabStripView
                                                   alignment:alignment];
  return [tabStripView convertPoint:position toView:windowView];
}

+ (NSPoint)themeImagePositionInTabStripCoords:(NSView*)tabStripView
                                    alignment:(ThemeImageAlignment)alignment {
  DCHECK(tabStripView);

  if (alignment == THEME_IMAGE_ALIGN_WITH_TAB_STRIP) {
    // The theme image is lined up with the top of the tab which is below the
    // top of the tab strip.
    return NSMakePoint(kPatternHorizontalOffset,
        [TabStripController defaultTabHeight] +
            GetPatternVerticalOffsetWithTabStrip(true));
  }
  // The theme image is lined up with the top of the tab strip (as opposed to
  // the top of the tab above). This is the same as lining up with the top of
  // the window's root view when not in presentation mode.
  return NSMakePoint(kPatternHorizontalOffset,
                     NSHeight([tabStripView bounds]) +
                         GetPatternVerticalOffsetWithTabStrip(false));
}

+ (void)activateWindowForController:(NSWindowController*)controller {
  // Per http://crbug.com/73779 and http://crbug.com/75223, we need this to
  // properly activate windows if Chrome is not the active application.
  [[controller window] makeKeyAndOrderFront:controller];
  [NSApp activateIgnoringOtherApps:YES];
}

@end
