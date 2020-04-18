// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/message_center/cocoa/notification_controller.h"

#include <stddef.h>

#include <algorithm>

#include "base/mac/foundation_util.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/url_formatter/elide_url.h"
#include "skia/ext/skia_utils_mac.h"
#import "ui/base/cocoa/hover_image_button.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/text_elider.h"
#include "ui/gfx/text_utils.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/message_center_style.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/strings/grit/ui_strings.h"
#include "url/gurl.h"

@interface MCNotificationProgressBar : NSProgressIndicator
@end

@implementation MCNotificationProgressBar
- (void)drawRect:(NSRect)dirtyRect {
  NSRect sliceRect, remainderRect;
  double progressFraction = ([self doubleValue] - [self minValue]) /
      ([self maxValue] - [self minValue]);
  NSDivideRect(dirtyRect, &sliceRect, &remainderRect,
               NSWidth(dirtyRect) * progressFraction, NSMinXEdge);

  NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:dirtyRect
      xRadius:message_center::kProgressBarCornerRadius
      yRadius:message_center::kProgressBarCornerRadius];
  [skia::SkColorToCalibratedNSColor(message_center::kProgressBarBackgroundColor)
      set];
  [path fill];

  if (progressFraction == 0.0)
    return;

  path = [NSBezierPath bezierPathWithRoundedRect:sliceRect
      xRadius:message_center::kProgressBarCornerRadius
      yRadius:message_center::kProgressBarCornerRadius];
  [skia::SkColorToCalibratedNSColor(message_center::kProgressBarSliceColor)
      set];
  [path fill];
}

- (id)accessibilityAttributeValue:(NSString*)attribute {
  double progressValue = 0.0;
  if ([attribute isEqualToString:NSAccessibilityDescriptionAttribute]) {
    progressValue = [self doubleValue];
  } else if ([attribute isEqualToString:NSAccessibilityMinValueAttribute]) {
    progressValue = [self minValue];
  } else if ([attribute isEqualToString:NSAccessibilityMaxValueAttribute]) {
    progressValue = [self maxValue];
  } else {
    return [super accessibilityAttributeValue:attribute];
  }

  return [NSString stringWithFormat:@"%lf", progressValue];
}
@end

////////////////////////////////////////////////////////////////////////////////
@interface MCNotificationButton : NSButton
@end

@implementation MCNotificationButton
// drawRect: needs to fill the button with a background, otherwise we don't get
// subpixel antialiasing.
- (void)drawRect:(NSRect)dirtyRect {
  NSColor* color = skia::SkColorToCalibratedNSColor(
      message_center::kNotificationBackgroundColor);
  [color set];
  NSRectFill(dirtyRect);
  [super drawRect:dirtyRect];
}
@end

@interface MCNotificationButtonCell : NSButtonCell {
  BOOL hovered_;
}
@end

////////////////////////////////////////////////////////////////////////////////
@implementation MCNotificationButtonCell
- (BOOL)isOpaque {
  return YES;
}

- (void)drawBezelWithFrame:(NSRect)frame inView:(NSView*)controlView {
  // Else mouseEntered: and mouseExited: won't be called and hovered_ won't be
  // valid.
  DCHECK([self showsBorderOnlyWhileMouseInside]);

  if (!hovered_)
    return;
  [skia::SkColorToCalibratedNSColor(
      message_center::kHoveredButtonBackgroundColor) set];
  NSRectFill(frame);
}

- (void)drawImage:(NSImage*)image
        withFrame:(NSRect)frame
           inView:(NSView*)controlView {
  if (!image)
    return;
  NSRect rect = NSMakeRect(message_center::kButtonHorizontalPadding,
                           message_center::kButtonIconTopPadding,
                           message_center::kNotificationButtonIconSize,
                           message_center::kNotificationButtonIconSize);
  [image drawInRect:rect
            fromRect:NSZeroRect
           operation:NSCompositeSourceOver
            fraction:1.0
      respectFlipped:YES
               hints:nil];
}

- (NSRect)drawTitle:(NSAttributedString*)title
          withFrame:(NSRect)frame
             inView:(NSView*)controlView {
  CGFloat offsetX = message_center::kButtonHorizontalPadding;
  if ([base::mac::ObjCCastStrict<NSButton>(controlView) image]) {
    offsetX += message_center::kNotificationButtonIconSize +
               message_center::kButtonIconToTitlePadding;
  }
  frame.origin.x = offsetX;
  frame.size.width -= offsetX;

  NSDictionary* attributes = @{
    NSFontAttributeName :
        [title attribute:NSFontAttributeName atIndex:0 effectiveRange:NULL],
    NSForegroundColorAttributeName :
        skia::SkColorToCalibratedNSColor(message_center::kRegularTextColor),
  };
  [[title string] drawWithRect:frame
                       options:(NSStringDrawingUsesLineFragmentOrigin |
                                NSStringDrawingTruncatesLastVisibleLine)
                    attributes:attributes];
  return frame;
}

- (void)mouseEntered:(NSEvent*)event {
  hovered_ = YES;

  // Else the cell won't be repainted on hover.
  [super mouseEntered:event];
}

- (void)mouseExited:(NSEvent*)event {
  hovered_ = NO;
  [super mouseExited:event];
}
@end

////////////////////////////////////////////////////////////////////////////////

@interface MCNotificationView : NSBox {
 @private
  MCNotificationController* controller_;
}

- (id)initWithController:(MCNotificationController*)controller
                   frame:(NSRect)frame;
@end

@implementation MCNotificationView
- (id)initWithController:(MCNotificationController*)controller
                   frame:(NSRect)frame {
  if ((self = [super initWithFrame:frame]))
    controller_ = controller;
  return self;
}

- (void)mouseUp:(NSEvent*)event {
  if (event.type != NSLeftMouseUp) {
    [super mouseUp:event];
    return;
  }
  if (NSPointInRect([self convertPoint:event.locationInWindow fromView:nil],
                    self.bounds)) {
    [controller_ notificationClicked];
  }
}

- (NSView*)hitTest:(NSPoint)point {
  // Route the mouse click events on NSTextView to the container view.
  NSView* hitView = [super hitTest:point];
  if (hitView)
    return [hitView isKindOfClass:[NSTextView class]] ? self : hitView;
  return nil;
}

- (BOOL)accessibilityIsIgnored {
  return NO;
}

- (NSArray*)accessibilityActionNames {
  return @[ NSAccessibilityPressAction ];
}

- (void)accessibilityPerformAction:(NSString*)action {
  if ([action isEqualToString:NSAccessibilityPressAction]) {
    [controller_ notificationClicked];
    return;
  }
  [super accessibilityPerformAction:action];
}
@end

////////////////////////////////////////////////////////////////////////////////

@interface AccessibilityIgnoredBox : NSBox
@end

// Ignore this element, but expose its children to accessibility.
@implementation AccessibilityIgnoredBox
- (BOOL)accessibilityIsIgnored {
  return YES;
}

// Pretend this element has no children.
// TODO(petewil): Until we have alt text available, we will hide the children of
//  the box also.  Remove this override once alt text is set (by using
// NSAccessibilityDescriptionAttribute).
- (id)accessibilityAttributeValue:(NSString*)attribute {
  // If we get a request for NSAccessibilityChildrenAttribute, return an empty
  // array to pretend we have no children.
  if ([attribute isEqualToString:NSAccessibilityChildrenAttribute])
    return @[];
  else
    return [super accessibilityAttributeValue:attribute];
}
@end

////////////////////////////////////////////////////////////////////////////////

@interface MCNotificationController (Private)
// Configures a NSBox to be borderless, titleless, and otherwise appearance-
// free.
- (void)configureCustomBox:(NSBox*)box;

// Initializes the icon_ ivar and returns the view to insert into the hierarchy.
- (NSView*)createIconView;

// Creates a box that shows a border when the icon is not big enough to fill the
// space.
- (NSBox*)createImageBox:(const gfx::Image&)notificationImage;

// Initializes the closeButton_ ivar with the configured button.
- (void)configureCloseButtonInFrame:(NSRect)rootFrame;

// Initializes the settingsButton_ ivar with the configured button.
- (void)configureSettingsButtonInFrame:(NSRect)rootFrame;

// Creates the smallImage_ ivar with the appropriate frame.
- (NSView*)createSmallImageInFrame:(NSRect)rootFrame;

// Initializes title_ in the given frame.
- (void)configureTitleInFrame:(NSRect)rootFrame;

// Initializes message_ in the given frame.
- (void)configureBodyInFrame:(NSRect)rootFrame;

// Initializes contextMessage_ in the given frame.
- (void)configureContextMessageInFrame:(NSRect)rootFrame;

// Creates a NSTextView that the caller owns configured as a label in a
// notification.
- (NSTextView*)newLabelWithFrame:(NSRect)frame;

// Gets the rectangle in which notification content should be placed. This
// rectangle is to the right of the icon and left of the control buttons.
// This depends on the icon_ and closeButton_ being initialized.
- (NSRect)currentContentRect;

// Returns the wrapped text that could fit within the content rect with not
// more than the given number of lines. The wrapped text would be painted using
// the given font. The Ellipsis could be added at the end of the last line if
// it is too long. Outputs the number of lines computed in the actualLines
// parameter.
- (base::string16)wrapText:(const base::string16&)text
                   forFont:(NSFont*)font
          maxNumberOfLines:(size_t)lines
               actualLines:(size_t*)actualLines;

// Same as above without outputting the lines formatted.
- (base::string16)wrapText:(const base::string16&)text
                   forFont:(NSFont*)font
          maxNumberOfLines:(size_t)lines;

@end

////////////////////////////////////////////////////////////////////////////////

@implementation MCNotificationController

- (id)initWithNotification:(const message_center::Notification*)notification
    messageCenter:(message_center::MessageCenter*)messageCenter {
  if ((self = [super initWithNibName:nil bundle:nil])) {
    notification_ = notification;
    notificationID_ = notification_->id();
    messageCenter_ = messageCenter;
  }
  return self;
}

- (void)loadView {
  // Create the root view of the notification.
  NSRect rootFrame = NSMakeRect(0, 0,
      message_center::kNotificationPreferredImageWidth,
      message_center::kNotificationIconSize);
  base::scoped_nsobject<MCNotificationView> rootView(
      [[MCNotificationView alloc] initWithController:self frame:rootFrame]);
  [self configureCustomBox:rootView];
  [rootView setFillColor:skia::SkColorToCalibratedNSColor(
      message_center::kNotificationBackgroundColor)];
  [self setView:rootView];

  [rootView addSubview:[self createIconView]];

  // Create the close button.
  [self configureCloseButtonInFrame:rootFrame];
  [rootView addSubview:closeButton_];

  // Create the small image.
  [rootView addSubview:[self createSmallImageInFrame:rootFrame]];

  // Create the settings button.
  if (notification_->should_show_settings_button()) {
    [self configureSettingsButtonInFrame:rootFrame];
    [rootView addSubview:settingsButton_];
  }

  NSRect contentFrame = [self currentContentRect];

  // Create the title.
  [self configureTitleInFrame:contentFrame];
  [rootView addSubview:title_];

  // Create the message body.
  [self configureBodyInFrame:contentFrame];
  [rootView addSubview:message_];

  // Create the context message body.
  [self configureContextMessageInFrame:contentFrame];
  [rootView addSubview:contextMessage_];

  // Populate the data.
  [self updateNotification:notification_];
}

- (NSRect)updateNotification:(const message_center::Notification*)notification {
  DCHECK_EQ(notification->id(), notificationID_);
  notification_ = notification;

  message_center::NotificationLayoutParams layoutParams;
  layoutParams.rootFrame =
      NSMakeRect(0, 0, message_center::kNotificationPreferredImageWidth,
                 message_center::kNotificationIconSize);

  [smallImage_ setImage:notification_->small_image().AsNSImage()];

  // Update the icon.
  [icon_ setImage:notification_->icon().AsNSImage()];

  // The message_center:: constants are relative to capHeight at the top and
  // relative to the baseline at the bottom, but NSTextField uses the full line
  // height for its height.
  CGFloat titleTopGap =
      roundf([[title_ font] ascender] - [[title_ font] capHeight]);
  CGFloat titleBottomGap = roundf(fabs([[title_ font] descender]));
  CGFloat titlePadding = message_center::kTextTopPadding - titleTopGap;

  CGFloat messageTopGap =
      roundf([[message_ font] ascender] - [[message_ font] capHeight]);
  CGFloat messageBottomGap = roundf(fabs([[message_ font] descender]));
  CGFloat messagePadding =
      message_center::kTextTopPadding - titleBottomGap - messageTopGap;

  CGFloat contextMessageTopGap = roundf(
      [[contextMessage_ font] ascender] - [[contextMessage_ font] capHeight]);
  CGFloat contextMessagePadding =
      message_center::kTextTopPadding - messageBottomGap - contextMessageTopGap;

  // Set the title and recalculate the frame.
  size_t actualTitleLines = 0;
  [title_ setString:base::SysUTF16ToNSString([self
                                wrapText:notification_->title()
                                 forFont:[title_ font]
                        maxNumberOfLines:message_center::kMaxTitleLines
                             actualLines:&actualTitleLines])];
  [title_ sizeToFit];
  layoutParams.titleFrame = [title_ frame];
  layoutParams.titleFrame.origin.y = NSMaxY(layoutParams.rootFrame) -
                                     titlePadding -
                                     NSHeight(layoutParams.titleFrame);

  // The number of message lines depends on the number of context message lines
  // and the lines within the title, and whether an image exists.
  int messageLineLimit = message_center::kMessageExpandedLineLimit;
  if (actualTitleLines > 1)
    messageLineLimit -= (actualTitleLines - 1) * 2;
  if (!notification_->image().IsEmpty()) {
    messageLineLimit /= 2;

    if (!notification_->context_message().empty() &&
        !notification_->UseOriginAsContextMessage())
      messageLineLimit -= message_center::kContextMessageLineLimit;
  }
  if (messageLineLimit < 0)
    messageLineLimit = 0;

  // Set the message and recalculate the frame.
  [message_ setString:base::SysUTF16ToNSString(
      [self wrapText:notification_->message()
             forFont:[message_ font]
      maxNumberOfLines:messageLineLimit])];
  [message_ sizeToFit];
  layoutParams.messageFrame = [message_ frame];

  // If there are list items, then the message_ view should not be displayed.
  const std::vector<message_center::NotificationItem>& items =
      notification->items();
  // If there are list items, don't show the main message.  Also if the message
  // is empty, mark it as hidden and set 0 height, so it doesn't take up any
  // space (size to fit leaves it 15 px tall.
  if (items.size() > 0 || notification_->message().empty()) {
    [message_ setHidden:YES];
    layoutParams.messageFrame.origin.y = layoutParams.titleFrame.origin.y;
    layoutParams.messageFrame.size.height = 0;
  } else {
    [message_ setHidden:NO];
    layoutParams.messageFrame.origin.y = NSMinY(layoutParams.titleFrame) -
                                         messagePadding -
                                         NSHeight(layoutParams.messageFrame);
    layoutParams.messageFrame.size.height = NSHeight([message_ frame]);
  }

  // Set the context message and recalculate the frame.
  base::string16 message;
  if (notification->UseOriginAsContextMessage()) {
    gfx::FontList font_list((gfx::Font([message_ font])));
    message = url_formatter::ElideHost(notification->origin_url(), font_list,
                                       message_center::kContextMessageViewWidth,
                                       gfx::Typesetter::NATIVE);
  } else {
    message = notification->context_message();
  }

  base::string16 elided =
      [self wrapText:message
             forFont:[contextMessage_ font]
          maxNumberOfLines:message_center::kContextMessageLineLimit];
  [contextMessage_ setString:base::SysUTF16ToNSString(elided)];
  [contextMessage_ sizeToFit];

  layoutParams.contextMessageFrame = [contextMessage_ frame];

  if (notification->context_message().empty() &&
      !notification->UseOriginAsContextMessage()) {
    [contextMessage_ setHidden:YES];
    layoutParams.contextMessageFrame.origin.y =
        layoutParams.messageFrame.origin.y;
    layoutParams.contextMessageFrame.size.height = 0;
  } else {
    [contextMessage_ setHidden:NO];

    // If the context message is used as a domain make sure it's placed at the
    // bottom of the top section.
    CGFloat contextMessageY = NSMinY(layoutParams.messageFrame) -
                              contextMessagePadding -
                              NSHeight(layoutParams.contextMessageFrame);
    layoutParams.contextMessageFrame.origin.y =
        notification->UseOriginAsContextMessage()
            ? std::min(NSMinY([icon_ frame]) + contextMessagePadding,
                       contextMessageY)
            : contextMessageY;
    layoutParams.contextMessageFrame.size.height =
        NSHeight([contextMessage_ frame]);
  }

  // Calculate the settings button position. It is dependent on whether the
  // context message aligns or not with the icon.
  layoutParams.settingsButtonFrame = [settingsButton_ frame];
  layoutParams.settingsButtonFrame.origin.y =
      MIN(NSMinY([icon_ frame]) + message_center::kSmallImagePadding,
          NSMinY(layoutParams.contextMessageFrame));

  // Create the list item views (up to a maximum).
  [listView_ removeFromSuperview];
  layoutParams.listFrame = NSZeroRect;
  if (items.size() > 0) {
    layoutParams.listFrame = [self currentContentRect];
    layoutParams.listFrame.origin.y = 0;
    layoutParams.listFrame.size.height = 0;
    listView_.reset([[NSView alloc] initWithFrame:layoutParams.listFrame]);
    [listView_ accessibilitySetOverrideValue:NSAccessibilityListRole
                                    forAttribute:NSAccessibilityRoleAttribute];
    [listView_
        accessibilitySetOverrideValue:NSAccessibilityContentListSubrole
                         forAttribute:NSAccessibilitySubroleAttribute];
    CGFloat y = 0;

    NSFont* font = [NSFont systemFontOfSize:message_center::kMessageFontSize];
    CGFloat lineHeight = roundf(NSHeight([font boundingRectForFont]));

    const int kNumNotifications =
        std::min(items.size(), message_center::kNotificationMaximumItems);
    for (int i = kNumNotifications - 1; i >= 0; --i) {
      NSTextView* itemView = [self
          newLabelWithFrame:NSMakeRect(0, y, NSWidth(layoutParams.listFrame),
                                       lineHeight)];
      [itemView setFont:font];

      // Disable the word-wrap in order to show the text in single line.
      [[itemView textContainer] setContainerSize:NSMakeSize(FLT_MAX, FLT_MAX)];
      [[itemView textContainer] setWidthTracksTextView:NO];

      // Construct the text from the title and message.
      base::string16 text =
          items[i].title + base::UTF8ToUTF16(" ") + items[i].message;
      base::string16 ellidedText =
          [self wrapText:text forFont:font maxNumberOfLines:1];
      [itemView setString:base::SysUTF16ToNSString(ellidedText)];

      // Use dim color for the title part.
      NSColor* titleColor =
          skia::SkColorToCalibratedNSColor(message_center::kRegularTextColor);
      NSRange titleRange = NSMakeRange(
          0,
          std::min(ellidedText.size(), items[i].title.size()));
      [itemView setTextColor:titleColor range:titleRange];

      // Use dim color for the message part if it has not been truncated.
      if (ellidedText.size() > items[i].title.size() + 1) {
        NSColor* messageColor =
            skia::SkColorToCalibratedNSColor(message_center::kDimTextColor);
        NSRange messageRange = NSMakeRange(
            items[i].title.size() + 1,
            ellidedText.size() - items[i].title.size() - 1);
        [itemView setTextColor:messageColor range:messageRange];
      }

      [listView_ addSubview:itemView];
      y += lineHeight;
    }
    // TODO(thakis): The spacing is not completely right.
    CGFloat listTopPadding =
        message_center::kTextTopPadding - contextMessageTopGap;
    layoutParams.listFrame.size.height = y;
    layoutParams.listFrame.origin.y = NSMinY(layoutParams.contextMessageFrame) -
                                      listTopPadding -
                                      NSHeight(layoutParams.listFrame);
    [listView_ setFrame:layoutParams.listFrame];
    [[self view] addSubview:listView_];
  }

  // Create the progress bar view if needed.
  [progressBarView_ removeFromSuperview];
  layoutParams.progressBarFrame = NSZeroRect;
  if (notification->type() == message_center::NOTIFICATION_TYPE_PROGRESS) {
    layoutParams.progressBarFrame = [self currentContentRect];
    layoutParams.progressBarFrame.origin.y =
        NSMinY(layoutParams.contextMessageFrame) -
        message_center::kProgressBarTopPadding -
        message_center::kProgressBarThickness;
    layoutParams.progressBarFrame.size.height =
        message_center::kProgressBarThickness;
    progressBarView_.reset([[MCNotificationProgressBar alloc]
        initWithFrame:layoutParams.progressBarFrame]);
    // Setting indeterminate to NO does not work with custom drawRect.
    [progressBarView_ setIndeterminate:YES];
    [progressBarView_ setStyle:NSProgressIndicatorBarStyle];
    [progressBarView_ setDoubleValue:notification->progress()];
    [[self view] addSubview:progressBarView_];
  }

  // If the bottom-most element so far is out of the rootView's bounds, resize
  // the view.
  CGFloat minY = NSMinY(layoutParams.contextMessageFrame);
  if (listView_ && NSMinY(layoutParams.listFrame) < minY)
    minY = NSMinY(layoutParams.listFrame);
  if (progressBarView_ && NSMinY(layoutParams.progressBarFrame) < minY)
    minY = NSMinY(layoutParams.progressBarFrame);
  if (minY < messagePadding) {
    CGFloat delta = messagePadding - minY;
    [self adjustFrameHeight:&layoutParams delta:delta];
  }

  // Add the bottom container view.
  NSRect frame = layoutParams.rootFrame;
  frame.size.height = 0;
  [bottomView_ removeFromSuperview];
  bottomView_.reset([[NSView alloc] initWithFrame:frame]);
  CGFloat y = 0;

  // Create action buttons if appropriate, bottom-up.
  std::vector<message_center::ButtonInfo> buttons = notification->buttons();
  for (int i = buttons.size() - 1; i >= 0; --i) {
    message_center::ButtonInfo buttonInfo = buttons[i];
    NSRect buttonFrame = frame;
    buttonFrame.origin = NSMakePoint(0, y);
    buttonFrame.size.height = message_center::kButtonHeight;
    base::scoped_nsobject<MCNotificationButton> button(
        [[MCNotificationButton alloc] initWithFrame:buttonFrame]);
    base::scoped_nsobject<MCNotificationButtonCell> cell(
        [[MCNotificationButtonCell alloc]
            initTextCell:base::SysUTF16ToNSString(buttonInfo.title)]);
    [cell setShowsBorderOnlyWhileMouseInside:YES];
    [button setCell:cell];
    [button setImage:buttonInfo.icon.AsNSImage()];
    [button setBezelStyle:NSSmallSquareBezelStyle];
    [button setImagePosition:NSImageLeft];
    [button setTag:i];
    [button setTarget:self];
    [button setAction:@selector(buttonClicked:)];
    y += NSHeight(buttonFrame);
    frame.size.height += NSHeight(buttonFrame);
    [bottomView_ addSubview:button];

    NSRect separatorFrame = frame;
    separatorFrame.origin = NSMakePoint(0, y);
    separatorFrame.size.height = 1;
    base::scoped_nsobject<NSBox> separator(
        [[AccessibilityIgnoredBox alloc] initWithFrame:separatorFrame]);
    [self configureCustomBox:separator];
    [separator setFillColor:skia::SkColorToCalibratedNSColor(
        message_center::kButtonSeparatorColor)];
    y += NSHeight(separatorFrame);
    frame.size.height += NSHeight(separatorFrame);
    [bottomView_ addSubview:separator];
  }

  // Create the image view if appropriate.
  gfx::Image notificationImage = notification->image();
  if (!notificationImage.IsEmpty()) {
    NSBox* imageBox = [self createImageBox:notificationImage];
    NSRect outerFrame = frame;
    outerFrame.origin = NSMakePoint(0, y);
    outerFrame.size = [imageBox frame].size;
    [imageBox setFrame:outerFrame];

    y += NSHeight(outerFrame);
    frame.size.height += NSHeight(outerFrame);

    [bottomView_ addSubview:imageBox];
  }

  [bottomView_ setFrame:frame];
  [[self view] addSubview:bottomView_];
  [self adjustFrameHeight:&layoutParams delta:NSHeight(frame)];

  // Make sure that there is a minimum amount of spacing below the icon and
  // the edge of the frame.
  CGFloat bottomDelta =
      NSHeight(layoutParams.rootFrame) - NSHeight([icon_ frame]);
  if (bottomDelta > 0 && bottomDelta < message_center::kIconBottomPadding) {
    CGFloat bottomAdjust = message_center::kIconBottomPadding - bottomDelta;
    [self adjustFrameHeight:&layoutParams delta:bottomAdjust];
  }

  [[self view] setFrame:layoutParams.rootFrame];
  [title_ setFrame:layoutParams.titleFrame];
  [message_ setFrame:layoutParams.messageFrame];
  [contextMessage_ setFrame:layoutParams.contextMessageFrame];
  [settingsButton_ setFrame:layoutParams.settingsButtonFrame];
  [listView_ setFrame:layoutParams.listFrame];
  [progressBarView_ setFrame:layoutParams.progressBarFrame];

  return layoutParams.rootFrame;
}

- (void)close:(id)sender {
  [closeButton_ setTarget:nil];
  messageCenter_->RemoveNotification([self notificationID], /*by_user=*/true);
}

- (void)settingsClicked:(id)sender {
  [NSApp activateIgnoringOtherApps:YES];
  messageCenter_->ClickOnSettingsButton([self notificationID]);
}

- (void)buttonClicked:(id)button {
  messageCenter_->ClickOnNotificationButton([self notificationID],
                                            [button tag]);
}

- (const message_center::Notification*)notification {
  return notification_;
}

- (const std::string&)notificationID {
  return notificationID_;
}

- (void)notificationClicked {
  messageCenter_->ClickOnNotification([self notificationID]);
}

- (void)adjustFrameHeight:(message_center::NotificationLayoutParams*)frames
                    delta:(CGFloat)delta {
  frames->rootFrame.size.height += delta;
  frames->titleFrame.origin.y += delta;
  frames->messageFrame.origin.y += delta;
  frames->contextMessageFrame.origin.y += delta;
  frames->settingsButtonFrame.origin.y += delta;
  frames->listFrame.origin.y += delta;
  frames->progressBarFrame.origin.y += delta;
}

// Private /////////////////////////////////////////////////////////////////////

- (void)configureCustomBox:(NSBox*)box {
  [box setBoxType:NSBoxCustom];
  [box setBorderType:NSNoBorder];
  [box setTitlePosition:NSNoTitle];
  [box setContentViewMargins:NSZeroSize];
}

- (NSView*)createIconView {
  // Create another box that shows a background color when the icon is not
  // big enough to fill the space.
  NSRect imageFrame = NSMakeRect(0, 0,
       message_center::kNotificationIconSize,
       message_center::kNotificationIconSize);
  base::scoped_nsobject<NSBox> imageBox(
      [[AccessibilityIgnoredBox alloc] initWithFrame:imageFrame]);
  [self configureCustomBox:imageBox];
  [imageBox setAutoresizingMask:NSViewMinYMargin];

  // Inside the image box put the actual icon view.
  icon_.reset([[NSImageView alloc] initWithFrame:imageFrame]);
  [imageBox setContentView:icon_];

  return imageBox.autorelease();
}

- (NSBox*)createImageBox:(const gfx::Image&)notificationImage {
  using message_center::kNotificationImageBorderSize;
  using message_center::kNotificationPreferredImageWidth;
  using message_center::kNotificationPreferredImageHeight;

  NSRect imageFrame = NSMakeRect(0, 0,
       kNotificationPreferredImageWidth,
       kNotificationPreferredImageHeight);
  base::scoped_nsobject<NSBox> imageBox(
      [[AccessibilityIgnoredBox alloc] initWithFrame:imageFrame]);
  [self configureCustomBox:imageBox];
  [imageBox setFillColor:skia::SkColorToCalibratedNSColor(
      message_center::kImageBackgroundColor)];

  // Images with non-preferred aspect ratios get a border on all sides.
  gfx::Size idealSize = gfx::Size(
      kNotificationPreferredImageWidth, kNotificationPreferredImageHeight);
  gfx::Size scaledSize = message_center::GetImageSizeForContainerSize(
      idealSize, notificationImage.Size());
  if (scaledSize != idealSize) {
    NSSize borderSize =
        NSMakeSize(kNotificationImageBorderSize, kNotificationImageBorderSize);
    [imageBox setContentViewMargins:borderSize];
  }

  NSImage* image = notificationImage.AsNSImage();
  base::scoped_nsobject<NSImageView> imageView(
      [[NSImageView alloc] initWithFrame:imageFrame]);
  [imageView setImage:image];
  [imageView setImageScaling:NSImageScaleProportionallyUpOrDown];
  [imageBox setContentView:imageView];

  return imageBox.autorelease();
}

- (void)configureCloseButtonInFrame:(NSRect)rootFrame {
  // The close button is configured to be the same size as the small image.
  int closeButtonOriginOffset =
      message_center::kSmallImageSize + message_center::kSmallImagePadding;
  NSRect closeButtonFrame =
      NSMakeRect(NSMaxX(rootFrame) - closeButtonOriginOffset,
                 NSMaxY(rootFrame) - closeButtonOriginOffset,
                 message_center::kSmallImageSize,
                 message_center::kSmallImageSize);
  closeButton_.reset([[HoverImageButton alloc] initWithFrame:closeButtonFrame]);
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  [closeButton_ setDefaultImage:
      rb.GetNativeImageNamed(IDR_NOTIFICATION_CLOSE).ToNSImage()];
  [closeButton_ setHoverImage:
      rb.GetNativeImageNamed(IDR_NOTIFICATION_CLOSE_HOVER).ToNSImage()];
  [closeButton_ setPressedImage:
      rb.GetNativeImageNamed(IDR_NOTIFICATION_CLOSE_PRESSED).ToNSImage()];
  [[closeButton_ cell] setHighlightsBy:NSOnState];
  [closeButton_ setTrackingEnabled:YES];
  [closeButton_ setBordered:NO];
  [closeButton_ setAutoresizingMask:NSViewMinYMargin];
  [closeButton_ setTarget:self];
  [closeButton_ setAction:@selector(close:)];
  [closeButton_ setDisableActivationOnClick:YES];
  [[closeButton_ cell]
      accessibilitySetOverrideValue:NSAccessibilityCloseButtonSubrole
                       forAttribute:NSAccessibilitySubroleAttribute];
  [[closeButton_ cell]
      accessibilitySetOverrideValue:
          l10n_util::GetNSString(IDS_APP_ACCNAME_CLOSE)
                       forAttribute:NSAccessibilityTitleAttribute];
}

- (void)configureSettingsButtonInFrame:(NSRect)rootFrame {
  // The settings button is configured to be the same size as the small image.
  int settingsButtonOriginOffset =
      message_center::kSmallImageSize + message_center::kSmallImagePadding;
  NSRect settingsButtonFrame = NSMakeRect(
      NSMaxX(rootFrame) - settingsButtonOriginOffset,
      message_center::kSmallImagePadding, message_center::kSmallImageSize,
      message_center::kSmallImageSize);

  settingsButton_.reset(
      [[HoverImageButton alloc] initWithFrame:settingsButtonFrame]);
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  [settingsButton_ setDefaultImage:rb.GetNativeImageNamed(
                                         IDR_NOTIFICATION_SETTINGS_BUTTON_ICON)
                                       .ToNSImage()];
  [settingsButton_
      setHoverImage:rb.GetNativeImageNamed(
                          IDR_NOTIFICATION_SETTINGS_BUTTON_ICON_HOVER)
                        .ToNSImage()];
  [settingsButton_
      setPressedImage:rb.GetNativeImageNamed(
                            IDR_NOTIFICATION_SETTINGS_BUTTON_ICON_PRESSED)
                          .ToNSImage()];
  [[settingsButton_ cell] setHighlightsBy:NSOnState];
  [settingsButton_ setTrackingEnabled:YES];
  [settingsButton_ setBordered:NO];
  [settingsButton_ setAutoresizingMask:NSViewMinYMargin];
  [settingsButton_ setTarget:self];
  [settingsButton_ setAction:@selector(settingsClicked:)];
  [[settingsButton_ cell]
      accessibilitySetOverrideValue:
          l10n_util::GetNSString(
              IDS_MESSAGE_NOTIFICATION_SETTINGS_BUTTON_ACCESSIBLE_NAME)
                       forAttribute:NSAccessibilityTitleAttribute];
}

- (NSView*)createSmallImageInFrame:(NSRect)rootFrame {
  int smallImageXOffset =
      message_center::kSmallImagePadding + message_center::kSmallImageSize;
  NSRect boxFrame =
      NSMakeRect(NSMaxX(rootFrame) - smallImageXOffset,
                 NSMinY(rootFrame) + message_center::kSmallImagePadding,
                 message_center::kSmallImageSize,
                 message_center::kSmallImageSize);

  // Put the smallImage inside another box which can hide it from accessibility
  // until we have some alt text to go with it.  Once we have alt text, remove
  // the box, and set NSAccessibilityDescriptionAttribute with it.
  base::scoped_nsobject<NSBox> imageBox(
      [[AccessibilityIgnoredBox alloc] initWithFrame:boxFrame]);
  [self configureCustomBox:imageBox];
  [imageBox setAutoresizingMask:NSViewMinYMargin];

  NSRect smallImageFrame =
      NSMakeRect(0,0,
                 message_center::kSmallImageSize,
                 message_center::kSmallImageSize);

  smallImage_.reset([[NSImageView alloc] initWithFrame:smallImageFrame]);
  [smallImage_ setImageScaling:NSImageScaleProportionallyUpOrDown];
  [imageBox setContentView:smallImage_];

  return imageBox.autorelease();
}

- (void)configureTitleInFrame:(NSRect)contentFrame {
  contentFrame.size.height = 0;
  title_.reset([self newLabelWithFrame:contentFrame]);
  [title_ setAutoresizingMask:NSViewMinYMargin];
  [title_ setTextColor:skia::SkColorToCalibratedNSColor(
      message_center::kRegularTextColor)];
  [title_ setFont:[NSFont messageFontOfSize:message_center::kTitleFontSize]];
}

- (void)configureBodyInFrame:(NSRect)contentFrame {
  contentFrame.size.height = 0;
  message_.reset([self newLabelWithFrame:contentFrame]);
  [message_ setAutoresizingMask:NSViewMinYMargin];
  [message_ setTextColor:skia::SkColorToCalibratedNSColor(
      message_center::kRegularTextColor)];
  [message_ setFont:
      [NSFont messageFontOfSize:message_center::kMessageFontSize]];
}

- (void)configureContextMessageInFrame:(NSRect)contentFrame {
  contentFrame.size.height = 0;
  contextMessage_.reset([self newLabelWithFrame:contentFrame]);
  [contextMessage_ setAutoresizingMask:NSViewMinYMargin];
  [contextMessage_ setTextColor:skia::SkColorToCalibratedNSColor(
      message_center::kDimTextColor)];
  [contextMessage_ setFont:
      [NSFont messageFontOfSize:message_center::kMessageFontSize]];
}

- (NSTextView*)newLabelWithFrame:(NSRect)frame {
  NSTextView* label = [[NSTextView alloc] initWithFrame:frame];

  // The labels MUST draw their background so that subpixel antialiasing can
  // happen on the text.
  [label setDrawsBackground:YES];
  [label setBackgroundColor:skia::SkColorToCalibratedNSColor(
      message_center::kNotificationBackgroundColor)];

  [label setEditable:NO];
  [label setSelectable:NO];
  [label setTextContainerInset:NSMakeSize(0.0f, 0.0f)];
  [[label textContainer] setLineFragmentPadding:0.0f];
  return label;
}

- (NSRect)currentContentRect {
  DCHECK(icon_);
  DCHECK(closeButton_);
  DCHECK(smallImage_);

  NSRect iconFrame, contentFrame;
  NSDivideRect([[self view] bounds], &iconFrame, &contentFrame,
      NSWidth([icon_ frame]) + message_center::kIconToTextPadding,
      NSMinXEdge);
  // The content area is between the icon on the left and the control area
  // on the right.
  int controlAreaWidth =
      std::max(NSWidth([closeButton_ frame]), NSWidth([smallImage_ frame]));
  contentFrame.size.width -=
      2 * message_center::kSmallImagePadding + controlAreaWidth;
  return contentFrame;
}

- (base::string16)wrapText:(const base::string16&)text
                   forFont:(NSFont*)nsfont
          maxNumberOfLines:(size_t)lines
               actualLines:(size_t*)actualLines {
  *actualLines = 0;
  if (text.empty() || lines == 0)
    return base::string16();
  gfx::FontList font_list((gfx::Font(nsfont)));
  int width = NSWidth([self currentContentRect]);
  int height = (lines + 1) * font_list.GetHeight();

  std::vector<base::string16> wrapped;
  gfx::ElideRectangleTextForNativeUi(text, font_list, width, height,
                                     gfx::WRAP_LONG_WORDS, &wrapped);

  // This could be possible when the input text contains only spaces.
  if (wrapped.empty())
    return base::string16();

  if (wrapped.size() > lines) {
    // Add an ellipsis to the last line. If this ellipsis makes the last line
    // too wide, that line will be further elided by the gfx::ElideText below.
    base::string16 last =
        wrapped[lines - 1] + base::UTF8ToUTF16(gfx::kEllipsis);
    if (gfx::GetStringWidth(last, font_list, gfx::Typesetter::NATIVE) > width) {
      last = gfx::ElideText(last, font_list, width, gfx::ELIDE_TAIL,
                            gfx::Typesetter::NATIVE);
    }
    wrapped.resize(lines - 1);
    wrapped.push_back(last);
  }

  *actualLines = wrapped.size();
  return lines == 1 ? wrapped[0]
                    : base::JoinString(wrapped, base::ASCIIToUTF16("\n"));
}

- (base::string16)wrapText:(const base::string16&)text
                   forFont:(NSFont*)nsfont
          maxNumberOfLines:(size_t)lines {
  size_t unused;
  return [self wrapText:text
                forFont:nsfont
       maxNumberOfLines:lines
            actualLines:&unused];
}

@end
