// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/content_settings/cookie_details_view_controller.h"

#include "base/mac/bundle_locations.h"
#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/content_settings/cookie_tree_node.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace {
static const int kExtraMarginBelowWhenExpirationEditable = 5;
}

#pragma mark View Controller

@implementation CookieDetailsViewController
@dynamic hasExpiration;

- (id)init {
  return [super initWithNibName:@"CookieDetailsView"
                         bundle:base::mac::FrameworkBundle()];
}

- (void)awakeFromNib {
  DCHECK(objectController_);
}

// Finds and returns the y offset of the lowest-most non-hidden
// text field in the view. This is used to shrink the view
// appropriately so that it just fits its visible content.
- (void)getLowestLabelVerticalPosition:(NSView*)view
                   lowestLabelPosition:(float&)lowestLabelPosition {
  if (![view isHidden]) {
    if ([view isKindOfClass:[NSTextField class]]) {
      NSRect frame = [view frame];
      if (frame.origin.y < lowestLabelPosition) {
        lowestLabelPosition = frame.origin.y;
      }
    }
    for (NSView* subview in [view subviews]) {
      [self getLowestLabelVerticalPosition:subview
                       lowestLabelPosition:lowestLabelPosition];
    }
  }
}

- (void)setContentObject:(id)content {
  // Make sure the view is loaded before we set the content object,
  // otherwise, the KVO notifications to update the content don't
  // reach the view and all of the detail values are default
  // strings.
  NSView* view = [self view];

  [objectController_ setValue:content forKey:@"content"];

  // View needs to be re-tweaked after setting the content object,
  // since the expiration date may have changed, changing the
  // size of the expiration popup.
  [tweaker_ tweakUI:view];
}

- (void)shrinkViewToFit {
  // Adjust the information pane to be exactly the right size
  // to hold the visible text information fields.
  NSView* view = [self view];
  NSRect frame = [view frame];
  float lowestLabelPosition = frame.origin.y + frame.size.height;
  [self getLowestLabelVerticalPosition:view
                   lowestLabelPosition:lowestLabelPosition];
  float verticalDelta = lowestLabelPosition - frame.origin.y;

  // Popup menu for the expiration is taller than the plain
  // text, give it some more room.
  if ([[[objectController_ content] details] canEditExpiration]) {
    verticalDelta -= kExtraMarginBelowWhenExpirationEditable;
  }

  frame.origin.y += verticalDelta;
  frame.size.height -= verticalDelta;
  [[self view] setFrame:frame];
}

- (void)configureBindingsForTreeController:(NSTreeController*)treeController {
  // There seems to be a bug in the binding logic that it's not possible
  // to bind to the selection of the tree controller, the bind seems to
  // require an additional path segment in the key, thus the use of
  // selection.self rather than just selection below.
  [objectController_ bind:@"contentObject"
                 toObject:treeController
              withKeyPath:@"selection.self"
                  options:nil];
}

- (IBAction)setCookieDoesntHaveExplicitExpiration:(id)sender {
  [[[objectController_ content] details] setHasExpiration:NO];
}

- (IBAction)setCookieHasExplicitExpiration:(id)sender {
  [[[objectController_ content] details] setHasExpiration:YES];
}

- (BOOL)hasExpiration {
  return [[[objectController_ content] details] hasExpiration];
}

@end
