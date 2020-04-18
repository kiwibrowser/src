// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/media_gallery_list_entry_view.h"

#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/ui/cocoa/chrome_style.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_control_utils.h"
#import "ui/base/cocoa/menu_controller.h"
#include "ui/base/resource/resource_bundle.h"

const CGFloat kCheckboxMargin = 10;

ui::MenuModel* MediaGalleryListEntryController::GetContextMenu(
    MediaGalleryPrefId pref_id) {
  return NULL;
}

@interface MediaGalleryListEntry ()
- (void)onCheckboxToggled:(id)sender;
- (void)onFolderViewerClicked:(id)sender;
- (ui::MenuModel*)getContextMenu;
- (void)layoutSubViews;
@end


@interface MediaGalleryButton : NSButton {
 @private
  MediaGalleryListEntry* controller_;  // |controller_| owns |self|.
  base::scoped_nsobject<MenuControllerCocoa> menuController_;
}

- (id)initWithFrame:(NSRect)frameRect
         controller:(MediaGalleryListEntry*)controller;
- (NSMenu*)menuForEvent:(NSEvent*)theEvent;

@end

@implementation MediaGalleryButton

- (id)initWithFrame:(NSRect)frameRect
         controller:(MediaGalleryListEntry*)controller {
  if ((self = [super initWithFrame:frameRect])) {
    controller_ = controller;
  }
  return self;
}

- (NSMenu*)menuForEvent:(NSEvent*)theEvent {
  menuController_.reset([[MenuControllerCocoa alloc]
               initWithModel:[controller_ getContextMenu]
      useWithPopUpButtonCell:NO]);
  return [menuController_ menu];
}

@end


@implementation MediaGalleryListEntry

- (id)initWithFrame:(NSRect)frameRect
         controller:(MediaGalleryListEntryController*)controller
           prefInfo:(const MediaGalleryPrefInfo&)prefInfo {
  if ((self = [super initWithFrame:frameRect])) {
    controller_ = controller;
    prefId_ = prefInfo.pref_id;

    NSString* nsTooltip =
        base::SysUTF16ToNSString(prefInfo.GetGalleryTooltip());

    // Set a auto resize mask so that -resizeWithOldSuperviewSize: is called.
    // It is overridden so the particular mask doesn't matter.
    [self setAutoresizingMask:NSViewWidthSizable];
    checkbox_.reset(
        [[MediaGalleryButton alloc] initWithFrame:NSZeroRect
                                       controller:self]);
    [[checkbox_ cell] setLineBreakMode:NSLineBreakByTruncatingMiddle];
    [checkbox_ setButtonType:NSSwitchButton];
    [checkbox_ setTarget:self];
    [checkbox_ setAction:@selector(onCheckboxToggled:)];

    [checkbox_ setTitle:
        base::SysUTF16ToNSString(prefInfo.GetGalleryDisplayName())];
    [checkbox_ setToolTip:nsTooltip];
    [self addSubview:checkbox_];

    // Additional details text.
    base::string16 subscript = prefInfo.GetGalleryAdditionalDetails();
    if (!subscript.empty()) {
      details_.reset([[NSTextField alloc] initWithFrame:NSZeroRect]);
      [[details_ cell] setLineBreakMode:NSLineBreakByTruncatingHead];
      [details_ setEditable:NO];
      [details_ setSelectable:NO];
      [details_ setBezeled:NO];
      [details_ setAttributedStringValue:
          constrained_window::GetAttributedLabelString(
              base::SysUTF16ToNSString(subscript),
              chrome_style::kTextFontStyle,
              NSNaturalTextAlignment,
              NSLineBreakByClipping
          )];
      [details_ setTextColor:[NSColor disabledControlTextColor]];
      [self addSubview:details_];
    }

    [self layoutSubViews];
  }
  return self;
}

- (void)setFrameSize:(NSSize)frameSize {
  [super setFrameSize:frameSize];
  [self layoutSubViews];
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldBoundsSize {
  NSRect frame = [self frame];
  frame.size.width = NSWidth([[self superview] frame]);
  [self setFrameSize:frame.size];
}

- (void)setState:(bool)selected {
  [checkbox_ setState:selected ? NSOnState : NSOffState];
}

- (void)onCheckboxToggled:(id)sender {
  controller_->OnCheckboxToggled(prefId_, [sender state] == NSOnState);
}

- (void)onFolderViewerClicked:(id)sender {
  controller_->OnFolderViewerClicked(prefId_);
}

- (ui::MenuModel*)getContextMenu {
 return controller_->GetContextMenu(prefId_);
}

- (void)layoutSubViews {
  NSRect bounds = [self bounds];
  // If we have an empty frame, we should auto size, so start with really big
  // bounds and then set it to the real size of the contents later.
  if (NSIsEmptyRect([self frame]))
    bounds.size = NSMakeSize(10000, 10000);

  [checkbox_ sizeToFit];
  [details_ sizeToFit];

  // Auto size everything and lay it out horizontally.
  CGFloat xPos = kCheckboxMargin;
  for (NSView* view in [self subviews]) {
    NSRect viewFrame = [view frame];
    viewFrame.origin.x = xPos;
    viewFrame.size.height = std::min(NSHeight(bounds), NSHeight(viewFrame));
    [view setFrame:viewFrame];
    xPos = NSMaxX([view frame]) + kCheckboxMargin;
  }

  // Size the views. If all the elements don't naturally fit, the checkbox
  // should get squished and will elide in the middle.  However, it shouldn't
  // squish too much so it gets at least half of the max width and the details
  // text should elide as well in that case.
  if (xPos > NSWidth(bounds)) {
    CGFloat maxRHSContent = NSWidth(bounds) / 2 - kCheckboxMargin;
    NSRect detailsFrame = [details_ frame];
    NSRect checkboxFrame = [checkbox_ frame];

    if (NSMaxX(detailsFrame) - NSMaxX(checkboxFrame) > maxRHSContent) {
      detailsFrame.size.width = std::max(
          maxRHSContent - (NSMinX(detailsFrame) - NSMaxX(checkboxFrame)),
          NSWidth(bounds) - kCheckboxMargin - NSMinX(detailsFrame));
      [details_ setFrameSize:detailsFrame.size];
      xPos = NSMaxX(detailsFrame) + kCheckboxMargin;
    }
    CGFloat overflow = xPos - NSWidth(bounds);
    if (overflow > 0) {
      checkboxFrame.size.width -= overflow;
      [checkbox_ setFrameSize:checkboxFrame.size];
      if (details_.get()) {
        detailsFrame.origin.x -= overflow;
        [details_ setFrameOrigin:detailsFrame.origin];
      }
    }
  }

  if (NSIsEmptyRect([self frame])) {
    NSRect frame = NSMakeRect(0, 0, 1, 1);
    for (NSView* view in [self subviews]) {
      frame = NSUnionRect(frame, [view frame]);
    }
    frame.size.width += kCheckboxMargin;
    [super setFrameSize:frame.size];
  }
}

@end
