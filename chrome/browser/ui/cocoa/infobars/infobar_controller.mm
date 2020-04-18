// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/infobars/infobar_controller.h"

#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "chrome/browser/infobars/infobar_service.h"
#import "chrome/browser/ui/cocoa/animatable_view.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/image_button_cell.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_background_view.h"
#include "chrome/browser/ui/cocoa/infobars/infobar_cocoa.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_container_cocoa.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_container_controller.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#include "chrome/grit/theme_resources.h"
#include "components/strings/grit/components_strings.h"
#import "ui/base/cocoa/controls/hyperlink_text_view.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/image/image.h"

@interface InfoBarController ()
// Sets |label_| based on |labelPlaceholder_|, sets |labelPlaceholder_| to nil.
- (void)initializeLabel;
@end

@implementation InfoBarController

- (id)initWithInfoBar:(InfoBarCocoa*)infobar {
  if ((self = [super initWithNibName:@"InfoBar"
                              bundle:base::mac::FrameworkBundle()])) {
    DCHECK(infobar);
    infobar_ = infobar->GetWeakPtr();
  }
  return self;
}

// All infobars have an icon, so we set up the icon in the base class
// awakeFromNib.
- (void)awakeFromNib {
  [[closeButton_ cell] setImageID:IDR_CLOSE_1
                   forButtonState:image_button_cell::kDefaultState];
  [[closeButton_ cell] setImageID:IDR_CLOSE_1_H
                   forButtonState:image_button_cell::kHoverState];
  [[closeButton_ cell] setImageID:IDR_CLOSE_1_P
                   forButtonState:image_button_cell::kPressedState];
  [[closeButton_ cell] setImageID:IDR_CLOSE_1
                   forButtonState:image_button_cell::kDisabledState];

  [closeButton_ setTitle:l10n_util::GetNSString(IDS_ACCNAME_CLOSE)];

  if (![self delegate]->GetIcon().IsEmpty()) {
    [image_ setImage:[self delegate]->GetIcon().ToNSImage()];
  } else {
    // No icon, remove it from the view and grow the textfield to include the
    // space.
    NSRect imageFrame = [image_ frame];
    NSRect labelFrame = [labelPlaceholder_ frame];
    labelFrame.size.width += NSMinX(imageFrame) - NSMinX(labelFrame);
    labelFrame.origin.x = imageFrame.origin.x;
    [image_ removeFromSuperview];
    image_ = nil;
    [labelPlaceholder_ setFrame:labelFrame];
  }
  [self initializeLabel];

  [self addAdditionalControls];

  [infoBarView_ setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  cocoa_l10n_util::FlipAllSubviewsIfNecessary(
      base::mac::ObjCCast<NSView>(infoBarView_));

  // TODO(ellyjones): InfoBar height should be computed from child heights +
  // appropriate (Harmony) margins, and children repositioned accordingly.
  constexpr int kDefaultBarTargetHeight = 40;
  infobar_->SetTargetHeight(kDefaultBarTargetHeight);
}

- (void)dealloc {
  [okButton_ setTarget:nil];
  [cancelButton_ setTarget:nil];
  [closeButton_ setTarget:nil];
  [super dealloc];
}

- (InfoBarCocoa*)infobar {
  return infobar_.get();
}

// Called when someone clicks on the embedded link.
- (BOOL)textView:(NSTextView*)textView
   clickedOnLink:(id)link
         atIndex:(NSUInteger)charIndex {
  if ([self respondsToSelector:@selector(linkClicked)])
    [self performSelector:@selector(linkClicked)];
  return YES;
}

- (BOOL)isOwned {
  return infobar_ && infobar_->OwnerCocoa() != NULL;
}

// Called when someone clicks on the ok button.
- (void)ok:(id)sender {
  // Subclasses must override this method if they do not hide the ok button.
  NOTREACHED();
}

// Called when someone clicks on the cancel button.
- (void)cancel:(id)sender {
  // Subclasses must override this method if they do not hide the cancel button.
  NOTREACHED();
}

// Called when someone clicks on the close button.
- (void)dismiss:(id)sender {
  if (![self isOwned])
    return;
  [self delegate]->InfoBarDismissed();
  [self removeSelf];
}

- (void)removeSelf {
  infobar_->RemoveSelf();
}

- (void)addAdditionalControls {
  // Default implementation does nothing.
}

- (void)infobarWillHide {
}

- (void)infobarWillClose {
}

- (void)removeButtons {
  // Extend the label all the way across.
  NSRect labelFrame = [label_.get() frame];
  labelFrame.size.width = NSMaxX([cancelButton_ frame]) - NSMinX(labelFrame);
  [okButton_ removeFromSuperview];
  okButton_ = nil;
  [cancelButton_ removeFromSuperview];
  cancelButton_ = nil;
  [label_.get() setFrame:labelFrame];
}

- (void)disablePopUpMenu:(NSMenu*)menu {
  // If the menu is re-opened, prevent queries to update items.
  [menu setDelegate:nil];

  // Prevent target/action messages to the controller.
  for (NSMenuItem* item in [menu itemArray]) {
    [item setEnabled:NO];
    [item setTarget:nil];
  }
}

- (infobars::InfoBarDelegate*)delegate {
  return infobar_->delegate();
}

- (void)initializeLabel {
  // Replace the label placeholder NSTextField with the real label NSTextView.
  // The former doesn't show links in a nice way, but the latter can't be added
  // in IB without a containing scroll view, so create the NSTextView
  // programmatically.
  base::scoped_nsobject<HyperlinkTextView> newLabel(
    [[HyperlinkTextView alloc] initWithFrame:[labelPlaceholder_ frame]]);
  [newLabel setDrawsBackgroundUsingSuperview:YES];
  [newLabel setAutoresizingMask:[labelPlaceholder_ autoresizingMask]];
  [[labelPlaceholder_ superview]
      replaceSubview:labelPlaceholder_ with:newLabel];
  labelPlaceholder_ = nil;  // Now released.
  [newLabel setDelegate:self];

  label_.reset(newLabel.release());
}

@end
