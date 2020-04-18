// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/webmenurunner_mac.h"

#include <stddef.h>

#include "base/strings/sys_string_conversions.h"

@interface WebMenuRunner (PrivateAPI)

// Worker function used during initialization.
- (void)addItem:(const content::MenuItem&)item;

// A callback for the menu controller object to call when an item is selected
// from the menu. This is not called if the menu is dismissed without a
// selection.
- (void)menuItemSelected:(id)sender;

@end  // WebMenuRunner (PrivateAPI)

@implementation WebMenuRunner

- (id)initWithItems:(const std::vector<content::MenuItem>&)items
           fontSize:(CGFloat)fontSize
       rightAligned:(BOOL)rightAligned {
  if ((self = [super init])) {
    menu_.reset([[NSMenu alloc] initWithTitle:@""]);
    [menu_ setAutoenablesItems:NO];
    index_ = -1;
    fontSize_ = fontSize;
    rightAligned_ = rightAligned;
    for (size_t i = 0; i < items.size(); ++i)
      [self addItem:items[i]];
  }
  return self;
}

- (void)addItem:(const content::MenuItem&)item {
  if (item.type == content::MenuItem::SEPARATOR) {
    [menu_ addItem:[NSMenuItem separatorItem]];
    return;
  }

  NSString* title = base::SysUTF16ToNSString(item.label);
  NSMenuItem* menuItem = [menu_ addItemWithTitle:title
                                          action:@selector(menuItemSelected:)
                                   keyEquivalent:@""];
  if (!item.tool_tip.empty()) {
    NSString* toolTip = base::SysUTF16ToNSString(item.tool_tip);
    [menuItem setToolTip:toolTip];
  }
  [menuItem setEnabled:(item.enabled && item.type != content::MenuItem::GROUP)];
  [menuItem setTarget:self];

  // Set various alignment/language attributes. Note that many (if not most) of
  // these attributes are functional only on 10.6 and above.
  base::scoped_nsobject<NSMutableDictionary> attrs(
      [[NSMutableDictionary alloc] initWithCapacity:3]);
  base::scoped_nsobject<NSMutableParagraphStyle> paragraphStyle(
      [[NSMutableParagraphStyle alloc] init]);
  [paragraphStyle setAlignment:rightAligned_ ? NSRightTextAlignment
                                             : NSLeftTextAlignment];
  NSWritingDirection writingDirection =
      item.rtl ? NSWritingDirectionRightToLeft
               : NSWritingDirectionLeftToRight;
  [paragraphStyle setBaseWritingDirection:writingDirection];
  [attrs setObject:paragraphStyle forKey:NSParagraphStyleAttributeName];

  if (item.has_directional_override) {
    base::scoped_nsobject<NSNumber> directionValue(
        [[NSNumber alloc] initWithInteger:
            writingDirection + NSTextWritingDirectionOverride]);
    base::scoped_nsobject<NSArray> directionArray(
        [[NSArray alloc] initWithObjects:directionValue.get(), nil]);
    [attrs setObject:directionArray forKey:NSWritingDirectionAttributeName];
  }

  [attrs setObject:[NSFont menuFontOfSize:fontSize_]
            forKey:NSFontAttributeName];

  base::scoped_nsobject<NSAttributedString> attrTitle(
      [[NSAttributedString alloc] initWithString:title attributes:attrs]);
  [menuItem setAttributedTitle:attrTitle];

  // We set the title as well as the attributed title here. The attributed title
  // will be displayed in the menu, but typeahead will use the non-attributed
  // string that doesn't contain any leading or trailing whitespace. This is
  // what Apple uses in WebKit as well:
  // http://trac.webkit.org/browser/trunk/Source/WebKit2/UIProcess/mac/WebPopupMenuProxyMac.mm#L90
  NSCharacterSet* whitespaceSet = [NSCharacterSet whitespaceCharacterSet];
  [menuItem setTitle:[title stringByTrimmingCharactersInSet:whitespaceSet]];

  [menuItem setTag:[menu_ numberOfItems] - 1];
}

// Reflects the result of the user's interaction with the popup menu. If NO, the
// menu was dismissed without the user choosing an item, which can happen if the
// user clicked outside the menu region or hit the escape key. If YES, the user
// selected an item from the menu.
- (BOOL)menuItemWasChosen {
  return menuItemWasChosen_;
}

- (void)menuItemSelected:(id)sender {
  menuItemWasChosen_ = YES;
}

- (void)runMenuInView:(NSView*)view
           withBounds:(NSRect)bounds
         initialIndex:(int)index {
  // Set up the button cell, converting to NSView coordinates. The menu is
  // positioned such that the currently selected menu item appears over the
  // popup button, which is the expected Mac popup menu behavior.
  base::scoped_nsobject<NSPopUpButtonCell> cell(
      [[NSPopUpButtonCell alloc] initTextCell:@"" pullsDown:NO]);
  [cell setMenu:menu_];
  // We use selectItemWithTag below so if the index is out-of-bounds nothing
  // bad happens.
  [cell selectItemWithTag:index];

  if (rightAligned_) {
    [cell setUserInterfaceLayoutDirection:
              NSUserInterfaceLayoutDirectionRightToLeft];
    // setUserInterfaceLayoutDirection for NSMenu is supported on macOS 10.11+.
    SEL sel = @selector(setUserInterfaceLayoutDirection:);
    if ([menu_ respondsToSelector:sel]) {
      NSUserInterfaceLayoutDirection direction =
          NSUserInterfaceLayoutDirectionRightToLeft;
      NSMethodSignature* signature =
          [NSMenu instanceMethodSignatureForSelector:sel];
      NSInvocation* invocation =
          [NSInvocation invocationWithMethodSignature:signature];
      [invocation setTarget:menu_.get()];
      [invocation setSelector:sel];
      [invocation setArgument:&direction atIndex:2];
      [invocation invoke];
    }
  }

  // When popping up a menu near the Dock, Cocoa restricts the menu
  // size to not overlap the Dock, with a scroll arrow.  Below a
  // certain point this doesn't work.  At that point the menu is
  // popped up above the element, so that the current item can be
  // selected without mouse-tracking selecting a different item
  // immediately.
  //
  // Unfortunately, instead of popping up above the passed |bounds|,
  // it pops up above the bounds of the view passed to inView:.  Use a
  // dummy view to fake this out.
  base::scoped_nsobject<NSView> dummyView(
      [[NSView alloc] initWithFrame:bounds]);
  [view addSubview:dummyView];

  // Display the menu, and set a flag if a menu item was chosen.
  [cell attachPopUpWithFrame:[dummyView bounds] inView:dummyView];
  [cell performClickWithFrame:[dummyView bounds] inView:dummyView];

  [dummyView removeFromSuperview];

  if ([self menuItemWasChosen])
    index_ = [cell indexOfSelectedItem];
}

- (void)hide {
  [menu_ cancelTracking];
}

- (int)indexOfSelectedItem {
  return index_;
}

@end  // WebMenuRunner
