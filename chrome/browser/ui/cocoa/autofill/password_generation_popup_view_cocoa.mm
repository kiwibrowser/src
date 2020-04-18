// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/autofill/password_generation_popup_view_cocoa.h"

#include <cmath>

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/autofill/autofill_popup_controller.h"
#include "chrome/browser/ui/autofill/autofill_popup_view.h"
#include "chrome/browser/ui/autofill/popup_constants.h"
#include "chrome/browser/ui/cocoa/autofill/password_generation_popup_view_bridge.h"
#include "chrome/browser/ui/cocoa/chrome_style.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "skia/ext/skia_utils_mac.h"
#import "ui/base/cocoa/controls/hyperlink_text_view.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/range/range.h"
#include "ui/gfx/text_constants.h"

using autofill::AutofillPopupView;
using autofill::PasswordGenerationPopupController;
using autofill::PasswordGenerationPopupView;
using base::scoped_nsobject;

namespace {

// The height of the divider between the password and help sections, in pixels.
const CGFloat kDividerHeight = 1;

// The amount of whitespace, in pixels, between lines of text in the password
// section.
const CGFloat kPasswordSectionVerticalSeparation = 5;

NSColor* DividerColor() {
  return skia::SkColorToCalibratedNSColor(
      PasswordGenerationPopupView::kDividerColor);
}

NSColor* HelpTextBackgroundColor() {
  return skia::SkColorToCalibratedNSColor(
      PasswordGenerationPopupView::kExplanatoryTextBackgroundColor);
}

NSColor* HelpTextColor() {
  return skia::SkColorToCalibratedNSColor(
      PasswordGenerationPopupView::kExplanatoryTextColor);
}

NSColor* HelpLinkColor() {
  return skia::SkColorToCalibratedNSColor(chrome_style::GetLinkColor());
}

}  // namespace

@implementation PasswordGenerationPopupViewCocoa

#pragma mark Initialisers

- (id)initWithFrame:(NSRect)frame {
  NOTREACHED();
  return nil;
}

- (id)initWithController:
    (autofill::PasswordGenerationPopupController*)controller
                   frame:(NSRect)frame {
  if (self = [super initWithDelegate:controller frame:frame]) {
    controller_ = controller;

    passwordSection_.reset([[NSView alloc] initWithFrame:NSZeroRect]);
    [self addSubview:passwordSection_];

    passwordField_.reset(
        [[self textFieldWithText:controller_->password()
                      attributes:[self passwordAttributes]] retain]);
    [passwordSection_ addSubview:passwordField_];

    passwordTitleField_.reset(
        [[self textFieldWithText:controller_->SuggestedText()
                      attributes:[self passwordTitleAttributes]] retain]);
    [passwordSection_ addSubview:passwordTitleField_];

    keyIcon_.reset([[NSImageView alloc] initWithFrame:NSZeroRect]);
    NSImage* keyImage = NSImageFromImageSkia(
        gfx::CreateVectorIcon(kKeyIcon, 16, gfx::kChromeIconGrey));

    [keyIcon_ setImage:keyImage];
    [passwordSection_ addSubview:keyIcon_];

    divider_.reset([[NSBox alloc] initWithFrame:NSZeroRect]);
    [divider_ setBoxType:NSBoxCustom];
    [divider_ setBorderType:NSLineBorder];
    [divider_ setBorderColor:DividerColor()];
    [self addSubview:divider_];

    helpTextView_.reset([[HyperlinkTextView alloc] initWithFrame:NSZeroRect]);
    [helpTextView_ setMessage:base::SysUTF16ToNSString(controller_->HelpText())
                     withFont:[self textFont]
                 messageColor:HelpTextColor()];
    [helpTextView_ addLinkRange:controller_->HelpTextLinkRange().ToNSRange()
                        withURL:nil
                      linkColor:HelpLinkColor()];
    [helpTextView_ setDelegate:self];
    [helpTextView_ setDrawsBackground:YES];
    [helpTextView_ setBackgroundColor:HelpTextBackgroundColor()];
    [helpTextView_
        setTextContainerInset:NSMakeSize(controller_->kHorizontalPadding,
                                         controller_->kHelpVerticalPadding)];
    // Remove the underlining.
    NSTextStorage* text = [helpTextView_ textStorage];
    [text addAttribute:NSUnderlineStyleAttributeName
                 value:@(NSUnderlineStyleNone)
                 range:controller_->HelpTextLinkRange().ToNSRange()];
    [self addSubview:helpTextView_];
}

  return self;
}

- (void)updateTrackingAreas {
  [super updateTrackingAreas];
  if (helpTextTrackingArea_.get())
    [self removeTrackingArea:helpTextTrackingArea_.get()];

  // Set up tracking for the help text so the cursor, etc. is properly handled.
  // Must set tracking to "always" because the autofill window is never key.
  NSTrackingAreaOptions options = NSTrackingActiveAlways |
                                  NSTrackingMouseEnteredAndExited |
                                  NSTrackingMouseMoved |
                                  NSTrackingCursorUpdate;
  helpTextTrackingArea_.reset(
      [[CrTrackingArea alloc] initWithRect:[self bounds]
                                   options:options
                                     owner:helpTextView_.get()
                                  userInfo:nil]);

  [self addTrackingArea:helpTextTrackingArea_.get()];
}

#pragma mark NSView implementation:

- (void)drawRect:(NSRect)dirtyRect {
  [super drawRect:dirtyRect];

  // If the view is in the process of being destroyed, don't bother drawing.
  if (!controller_)
    return;

  [self drawBackgroundAndBorder];

  if (controller_->password_selected()) {
    // Draw a highlight under the suggested password.
    NSRect highlightBounds = [passwordSection_ frame];
    [[self highlightColor] set];
    [NSBezierPath fillRect:highlightBounds];
  }
}

#pragma mark Public API:

- (NSSize)preferredSize {
  const NSSize passwordTitleSize =
      [base::SysUTF16ToNSString(controller_->SuggestedText())
          sizeWithAttributes:@{ NSFontAttributeName : [self boldFont] }];
  const NSSize passwordSize = [base::SysUTF16ToNSString(controller_->password())
      sizeWithAttributes:@{ NSFontAttributeName : [self textFont] }];

  CGFloat width =
      autofill::kPopupBorderThickness +
      controller_->kHorizontalPadding +
      [[keyIcon_ image] size].width +
      controller_->kHorizontalPadding +
      std::max(passwordSize.width, passwordTitleSize.width) +
      controller_->kHorizontalPadding +
      autofill::kPopupBorderThickness;

  width = std::max(width, (CGFloat)controller_->GetMinimumWidth());
  CGFloat contentWidth = width - (2 * controller_->kHorizontalPadding);

  CGFloat height =
      autofill::kPopupBorderThickness +
      controller_->kHelpVerticalPadding +
      [self helpSizeForPopupWidth:contentWidth].height +
      controller_->kHelpVerticalPadding +
      autofill::kPopupBorderThickness;

  if (controller_->display_password())
    height += controller_->kPopupPasswordSectionHeight;

  return NSMakeSize(width, height);
}

- (void)updateBoundsAndRedrawPopup {
  const CGFloat popupWidth = controller_->popup_bounds().width();
  const CGFloat contentWidth =
      popupWidth - (2 * autofill::kPopupBorderThickness);
  const CGFloat contentHeight = controller_->popup_bounds().height() -
                                (2 * autofill::kPopupBorderThickness);

  if (controller_->display_password()) {
    // The password can change while the bubble is shown: If the user has
    // accepted the password and then selects the form again and starts deleting
    // the password, the field will be initially invisible and then become
    // visible.
    [self updatePassword];

    // Lay out the password section, which includes the key icon, the title, and
    // the suggested password.
    [passwordSection_
        setFrame:NSMakeRect(autofill::kPopupBorderThickness,
                            autofill::kPopupBorderThickness,
                            contentWidth,
                            controller_->kPopupPasswordSectionHeight)];

    // The key icon falls to the left of the title and password.
    const NSSize imageSize = [[keyIcon_ image] size];
    const CGFloat keyX = controller_->kHorizontalPadding;
    const CGFloat keyY =
        std::ceil((controller_->kPopupPasswordSectionHeight / 2.0) -
                  (imageSize.height / 2.0));
    [keyIcon_ setFrame:{ NSMakePoint(keyX, keyY), imageSize }];

    // The title and password fall to the right of the key icon and are centered
    // vertically as a group with some padding in between.
    [passwordTitleField_ sizeToFit];
    [passwordField_ sizeToFit];
    const CGFloat groupHeight = NSHeight([passwordField_ frame]) +
                                kPasswordSectionVerticalSeparation +
                                NSHeight([passwordTitleField_ frame]);
    const CGFloat groupX =
        NSMaxX([keyIcon_ frame]) + controller_->kHorizontalPadding;
    const CGFloat groupY =
        std::ceil((controller_->kPopupPasswordSectionHeight / 2.0) -
                  (groupHeight / 2.0));
    [passwordField_ setFrameOrigin:NSMakePoint(groupX, groupY)];
    const CGFloat titleY = groupY +
                           NSHeight([passwordField_ frame]) +
                           kPasswordSectionVerticalSeparation;
    [passwordTitleField_ setFrameOrigin:NSMakePoint(groupX, titleY)];

    // Layout the divider, which falls immediately below the password section.
    const CGFloat dividerX = autofill::kPopupBorderThickness;
    const CGFloat dividerY = NSMaxY([passwordSection_ frame]);
    NSRect dividerFrame =
        NSMakeRect(dividerX, dividerY, contentWidth, kDividerHeight);
    [divider_ setFrame:dividerFrame];
  }

  // Layout the help section beneath the divider (if applicable, otherwise
  // beneath the border).
  const CGFloat helpX = autofill::kPopupBorderThickness;
  const CGFloat helpY = controller_->display_password()
      ? NSMaxY([divider_ frame])
      : autofill::kPopupBorderThickness;
  const CGFloat helpHeight = contentHeight -
                             NSHeight([passwordSection_ frame]) -
                             NSHeight([divider_ frame]);
  [helpTextView_ setFrame:NSMakeRect(helpX, helpY, contentWidth, helpHeight)];

  [super updateBoundsAndRedrawPopup];
}

- (BOOL)isPointInPasswordBounds:(NSPoint)point {
  return NSPointInRect(point, [passwordSection_ frame]);
}

- (void)controllerDestroyed {
  controller_ = NULL;
  [super delegateDestroyed];
}

#pragma mark NSTextViewDelegate implementation:

- (BOOL)textView:(NSTextView*)textView
   clickedOnLink:(id)link
         atIndex:(NSUInteger)charIndex {
  controller_->OnSavedPasswordsLinkClicked();
  return YES;
}

#pragma mark Private helpers:

- (void)updatePassword {
  base::scoped_nsobject<NSMutableAttributedString> updatedPassword(
      [[NSMutableAttributedString alloc]
          initWithString:base::SysUTF16ToNSString(controller_->password())
              attributes:[self passwordAttributes]]);
  [passwordField_ setAttributedStringValue:updatedPassword];
}

- (NSDictionary*)passwordTitleAttributes {
  scoped_nsobject<NSMutableParagraphStyle> paragraphStyle(
      [[NSMutableParagraphStyle alloc] init]);
  [paragraphStyle setAlignment:NSLeftTextAlignment];
  return @{
    NSFontAttributeName : [self boldFont],
    NSForegroundColorAttributeName : [self nameColor],
    NSParagraphStyleAttributeName : paragraphStyle.autorelease()
  };
}

- (NSDictionary*)passwordAttributes {
  scoped_nsobject<NSMutableParagraphStyle> paragraphStyle(
      [[NSMutableParagraphStyle alloc] init]);
  [paragraphStyle setAlignment:NSLeftTextAlignment];
  return @{
    NSFontAttributeName : [self textFont],
    NSForegroundColorAttributeName : [self nameColor],
    NSParagraphStyleAttributeName : paragraphStyle.autorelease()
  };
}

- (NSTextField*)textFieldWithText:(const base::string16&)text
                       attributes:(NSDictionary*)attributes {
  NSTextField* textField =
      [[[NSTextField alloc] initWithFrame:NSZeroRect] autorelease];
  scoped_nsobject<NSAttributedString> attributedString(
      [[NSAttributedString alloc]
          initWithString:base::SysUTF16ToNSString(text)
              attributes:attributes]);
  [textField setAttributedStringValue:attributedString.autorelease()];
  [textField setEditable:NO];
  [textField setSelectable:NO];
  [textField setDrawsBackground:NO];
  [textField setBezeled:NO];
  return textField;
}

- (NSSize)helpSizeForPopupWidth:(CGFloat)width {
  const CGFloat helpWidth = width -
                            2 * controller_->kHorizontalPadding -
                            2 * autofill::kPopupBorderThickness;
  const NSSize size = NSMakeSize(helpWidth, MAXFLOAT);
  NSRect textFrame = [base::SysUTF16ToNSString(controller_->HelpText())
      boundingRectWithSize:size
                   options:NSLineBreakByWordWrapping |
                           NSStringDrawingUsesLineFragmentOrigin
                attributes:@{ NSFontAttributeName : [self textFont] }];
  return textFrame.size;
}

- (NSFont*)boldFont {
  return [NSFont boldSystemFontOfSize:[NSFont smallSystemFontSize]];
}

- (NSFont*)textFont {
  return [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
}

@end
