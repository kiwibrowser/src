// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/toolbar_actions_bar_bubble_mac.h"

#include <utility>

#include "base/mac/bind_objc_block.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar_bubble_delegate.h"
#include "skia/ext/skia_utils_mac.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#include "third_party/skia/include/core/SkColor.h"
#import "ui/base/cocoa/controls/hyperlink_button_cell.h"
#import "ui/base/cocoa/hover_button.h"
#import "ui/base/cocoa/window_size_constants.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"

namespace {
BOOL g_animations_enabled = false;
CGFloat kMinWidth = 320.0;
}

@interface ToolbarActionsBarBubbleMac ()

// Handles the notification that the window will close.
- (void)windowWillClose:(NSNotification*)notification;

// Creates and returns an NSAttributed string with the specified size and
// alignment.
- (NSAttributedString*)attributedStringWithString:(const base::string16&)string
                                         fontSize:(CGFloat)fontSize
                                        alignment:(NSTextAlignment)alignment;

// Creates an NSTextField with the given string, size, and alignment, and adds
// it to the window.
- (NSTextField*)addTextFieldWithString:(const base::string16&)string
                              fontSize:(CGFloat)fontSize
                             alignment:(NSTextAlignment)alignment;

// Creates an ExtensionMessagebubbleButton the given string id, and adds it to
// the window.
- (NSButton*)addButtonWithString:(const base::string16&)string;

// Initializes the bubble's content.
- (void)layout;

// Handles a button being clicked.
- (void)onButtonClicked:(id)sender;

@end

@implementation ToolbarActionsBarBubbleMac

@synthesize actionButton = actionButton_;
@synthesize bodyText = bodyText_;
@synthesize itemList = itemList_;
@synthesize dismissButton = dismissButton_;
@synthesize link = link_;
@synthesize label = label_;
@synthesize iconView = iconView_;

- (id)initWithParentWindow:(NSWindow*)parentWindow
               anchorPoint:(NSPoint)anchorPoint
          anchoredToAction:(BOOL)anchoredToAction
                  delegate:
                      (std::unique_ptr<ToolbarActionsBarBubbleDelegate>)
                          delegate {
  base::scoped_nsobject<InfoBubbleWindow> window(
      [[InfoBubbleWindow alloc]
          initWithContentRect:ui::kWindowSizeDeterminedLater
                    styleMask:NSBorderlessWindowMask
                      backing:NSBackingStoreBuffered
                        defer:NO]);
  if ((self = [super initWithWindow:window
                       parentWindow:parentWindow
                         anchoredAt:anchorPoint])) {
    acknowledged_ = NO;
    anchoredToAction_ = anchoredToAction;
    delegate_ = std::move(delegate);

    ui::NativeTheme* nativeTheme = ui::NativeTheme::GetInstanceForNativeUi();
    [[self bubble] setAlignment:info_bubble::kAlignArrowToAnchor];
    [[self bubble] setArrowLocation:info_bubble::kTopTrailing];
    [[self bubble] setBackgroundColor:
        skia::SkColorToCalibratedNSColor(nativeTheme->GetSystemColor(
            ui::NativeTheme::kColorId_DialogBackground))];

    if (!g_animations_enabled)
      [window setAllowedAnimations:info_bubble::kAnimateNone];

    [self setShouldCloseOnResignKey:delegate_->ShouldCloseOnDeactivate()];

    [self layout];

    [[self window] makeFirstResponder:
        (actionButton_ ? actionButton_ : dismissButton_)];
  }
  return self;
}

+ (void)setAnimationEnabledForTesting:(BOOL)enabled {
  g_animations_enabled = enabled;
}

- (IBAction)showWindow:(id)sender {
  // The capturing lambda below is safe because the block creates a strong
  // reference to |self|. The Block_copy operator automatically retains object
  // variables, and when the block is destroyed, the variables are automatically
  // released.
  delegate_->OnBubbleShown(base::BindBlock(^{
    [self close];
  }));
  [super showWindow:sender];
}

// Private /////////////////////////////////////////////////////////////////////

- (void)windowWillClose:(NSNotification*)notification {
  if (!acknowledged_) {
    delegate_->OnBubbleClosed(
        ToolbarActionsBarBubbleDelegate::CLOSE_DISMISS_DEACTIVATION);
    acknowledged_ = YES;
  }
  // Deallocation happens asynchronously in Cocoa, but that makes testing
  // difficult. Explicitly destroy |delegate_| here so it can perform any
  // necessary cleanup.
  delegate_.reset();
  [super windowWillClose:notification];
}

- (NSAttributedString*)attributedStringWithString:(const base::string16&)string
                                         fontSize:(CGFloat)fontSize
                                        alignment:(NSTextAlignment)alignment {
  NSString* cocoaString = base::SysUTF16ToNSString(string);
  base::scoped_nsobject<NSMutableParagraphStyle> paragraphStyle(
      [[NSMutableParagraphStyle alloc] init]);
  [paragraphStyle setAlignment:alignment];
  NSDictionary* attributes = @{
    NSFontAttributeName : [NSFont systemFontOfSize:fontSize],
    NSForegroundColorAttributeName :
        [NSColor colorWithCalibratedWhite:0.2 alpha:1.0],
    NSParagraphStyleAttributeName : paragraphStyle.get()
  };
  return [[[NSAttributedString alloc] initWithString:cocoaString
                                          attributes:attributes] autorelease];
}

- (NSTextField*)addTextFieldWithString:(const base::string16&)string
                              fontSize:(CGFloat)fontSize
                             alignment:(NSTextAlignment)alignment {
  NSAttributedString* attributedString =
      [self attributedStringWithString:string
                              fontSize:fontSize
                             alignment:alignment];

  NSTextField* textField =
      [[[NSTextField alloc] initWithFrame:NSZeroRect] autorelease];
  [textField setEditable:NO];
  [textField setBordered:NO];
  [textField setDrawsBackground:NO];
  [textField setAttributedStringValue:attributedString];
  [[[self window] contentView] addSubview:textField];
  [textField sizeToFit];
  return textField;
}

- (NSButton*)addButtonWithString:(const base::string16&)string {
  NSButton* button = [[[NSButton alloc] initWithFrame:NSZeroRect] autorelease];
  NSAttributedString* buttonString =
      [self attributedStringWithString:string
                              fontSize:13.0
                             alignment:NSCenterTextAlignment];
  [button setAttributedTitle:buttonString];
  [button setBezelStyle:NSRoundedBezelStyle];
  [button setTarget:self];
  [button setAction:@selector(onButtonClicked:)];
  [[[self window] contentView] addSubview:button];
  [button sizeToFit];
  return button;
}

- (void)layout {
  // First, construct the pieces of the bubble that have a fixed width: the
  // heading, and the button strip (the extra view (icon and/or (linked) text),
  // the action button, and the dismiss button).
  NSTextField* heading =
      [self addTextFieldWithString:delegate_->GetHeadingText()
                          fontSize:13.0
                         alignment:NSLeftTextAlignment];
  NSSize headingSize = [heading frame].size;

  NSSize extraViewIconSize = NSZeroSize;
  NSSize extraViewTextSize = NSZeroSize;

  std::unique_ptr<ToolbarActionsBarBubbleDelegate::ExtraViewInfo>
      extra_view_info = delegate_->GetExtraViewInfo();

  if (extra_view_info) {
    // The extra view icon is optional.
    if (extra_view_info->resource) {
      NSImage* image =
          gfx::Image(gfx::CreateVectorIcon(*extra_view_info->resource, 16,
                                           gfx::kChromeIconGrey))
              .ToNSImage();
      NSRect frame = NSMakeRect(0, 0, image.size.width, image.size.height);
      iconView_ = [[[NSImageView alloc] initWithFrame:frame] autorelease];
      [iconView_ setImage:image];
      extraViewIconSize = frame.size;

      [[[self window] contentView] addSubview:iconView_];
    }

    const base::string16& text = extra_view_info->text;
    if (!text.empty()) {  // The extra view text is optional.
      if (extra_view_info->is_learn_more) {
        NSAttributedString* linkString =
            [self attributedStringWithString:text
                                    fontSize:13.0
                                   alignment:NSLeftTextAlignment];
        link_ = [HyperlinkButtonCell buttonWithString:linkString.string];
        [link_ setTarget:self];
        [link_ setAction:@selector(onButtonClicked:)];
        [[[self window] contentView] addSubview:link_];
        [link_ sizeToFit];
      } else {
        label_ = [self addTextFieldWithString:text
                                     fontSize:13.0
                                    alignment:NSLeftTextAlignment];
      }
      extraViewTextSize = label_ ? [label_ frame].size : [link_ frame].size;
    }
  }

  base::string16 cancelStr = delegate_->GetDismissButtonText();
  NSSize dismissButtonSize = NSZeroSize;
  if (!cancelStr.empty()) {  // A cancel/dismiss button is optional.
    dismissButton_ = [self addButtonWithString:cancelStr];
    dismissButtonSize =
        NSMakeSize(NSWidth([dismissButton_ frame]),
                   NSHeight([dismissButton_ frame]));
  }

  base::string16 actionStr = delegate_->GetActionButtonText();
  NSSize actionButtonSize = NSZeroSize;
  if (!actionStr.empty()) {  // The action button is optional.
    actionButton_ = [self addButtonWithString:actionStr];
    actionButtonSize =
        NSMakeSize(NSWidth([actionButton_ frame]),
                   NSHeight([actionButton_ frame]));
  }

  DCHECK(actionButton_ || dismissButton_);
  // TODO(devlin): This doesn't currently take into account
  // delegate_->GetDefaultDialogButton().

  CGFloat buttonStripHeight =
      std::max(actionButtonSize.height, dismissButtonSize.height);

  const CGFloat kButtonPadding = 5.0;
  CGFloat buttonStripWidth = 0;
  if (actionButton_)
    buttonStripWidth += actionButtonSize.width + kButtonPadding;
  if (dismissButton_)
    buttonStripWidth += dismissButtonSize.width + kButtonPadding;
  if (iconView_)
    buttonStripWidth += extraViewIconSize.width + kButtonPadding;
  if (link_ || label_)
    buttonStripWidth += extraViewTextSize.width + kButtonPadding;

  CGFloat headingWidth = headingSize.width;
  CGFloat windowWidth =
      std::max(std::max(kMinWidth, buttonStripWidth), headingWidth);

  base::string16 bodyTextString = delegate_->GetBodyText(anchoredToAction_);
  NSSize bodyTextSize;
  if (!bodyTextString.empty()) {
    bodyText_ = [self addTextFieldWithString:bodyTextString
                                    fontSize:12.0
                                   alignment:NSLeftTextAlignment];
    [bodyText_ setFrame:NSMakeRect(0, 0, windowWidth, 0)];
    // The content should have the same (max) width as the heading, which means
    // the text will most likely wrap.
    bodyTextSize =
        NSMakeSize(windowWidth, [GTMUILocalizerAndLayoutTweaker
                                    sizeToFitFixedWidthTextField:bodyText_]);
  }

  const CGFloat kItemListIndentation = 10.0;
  base::string16 itemListStr = delegate_->GetItemListText();
  NSSize itemListSize;
  if (!itemListStr.empty()) {
    itemList_ =
        [self addTextFieldWithString:itemListStr
                            fontSize:12.0
                           alignment:NSLeftTextAlignment];
    CGFloat listWidth = windowWidth - kItemListIndentation;
    [itemList_ setFrame:NSMakeRect(0, 0, listWidth, 0)];
    itemListSize = NSMakeSize(listWidth,
                              [GTMUILocalizerAndLayoutTweaker
                                   sizeToFitFixedWidthTextField:itemList_]);
  }

  const CGFloat kHorizontalPadding = 15.0;
  const CGFloat kVerticalPadding = 10.0;

  // Next, we set frame for all the different pieces of the bubble, from bottom
  // to top.
  windowWidth += kHorizontalPadding * 2;
  CGFloat currentHeight = kVerticalPadding;
  CGFloat currentMaxWidth = windowWidth - kHorizontalPadding;
  if (actionButton_) {
    [actionButton_ setFrame:NSMakeRect(
        currentMaxWidth - actionButtonSize.width,
        currentHeight,
        actionButtonSize.width,
        actionButtonSize.height)];
    currentMaxWidth -= (actionButtonSize.width + kButtonPadding);
  }
  if (dismissButton_) {
    [dismissButton_ setFrame:NSMakeRect(
        currentMaxWidth - dismissButtonSize.width,
        currentHeight,
        dismissButtonSize.width,
        dismissButtonSize.height)];
    currentMaxWidth -= (dismissButtonSize.width + kButtonPadding);
  }
  int leftAlignXPos = kHorizontalPadding;
  if (iconView_) {
    CGFloat extraViewIconHeight =
        currentHeight + (buttonStripHeight - extraViewIconSize.height) / 2.0;

    [iconView_
        setFrame:NSMakeRect(leftAlignXPos, extraViewIconHeight,
                            extraViewIconSize.width, extraViewIconSize.height)];
    leftAlignXPos += extraViewIconSize.width + kButtonPadding;
  }
  if (label_ || link_) {
    CGFloat extraViewTextHeight =
        currentHeight + (buttonStripHeight - extraViewTextSize.height) / 2.0;
    NSRect frame =
        NSMakeRect(leftAlignXPos, extraViewTextHeight, extraViewTextSize.width,
                   extraViewTextSize.height);
    if (link_) {
      [link_ setFrame:frame];
    } else {
      [label_ setFrame:frame];
    }
  }
  // Buttons have some inherit padding of their own, so we don't need quite as
  // much space here.
  currentHeight += buttonStripHeight + kVerticalPadding / 2;

  if (itemList_) {
    [itemList_ setFrame:NSMakeRect(kHorizontalPadding + kItemListIndentation,
                                   currentHeight,
                                   itemListSize.width,
                                   itemListSize.height)];
    currentHeight += itemListSize.height + kVerticalPadding;
  }

  if (bodyText_) {
    [bodyText_ setFrame:NSMakeRect(kHorizontalPadding, currentHeight,
                                   bodyTextSize.width, bodyTextSize.height)];
    currentHeight += bodyTextSize.height + kVerticalPadding;
  }

  [heading setFrame:NSMakeRect(kHorizontalPadding,
                               currentHeight,
                               headingSize.width,
                               headingSize.height)];

  // Update window frame.
  NSRect windowFrame = [[self window] frame];
  NSSize windowSize =
      NSMakeSize(windowWidth,
                 currentHeight + headingSize.height + kVerticalPadding * 2);
  // We need to convert the size to be in the window's coordinate system. Since
  // all we're doing is converting a size, and all views within a window share
  // the same size metrics, it's okay that the size calculation came from
  // multiple different views. Pick a view to convert it.
  windowSize = [heading convertSize:windowSize toView:nil];
  windowFrame.size = windowSize;
  [[self window] setFrame:windowFrame display:YES];
}

- (void)onButtonClicked:(id)sender {
  if (acknowledged_)
    return;
  ToolbarActionsBarBubbleDelegate::CloseAction action =
      ToolbarActionsBarBubbleDelegate::CLOSE_EXECUTE;
  if (link_ && sender == link_) {
    action = ToolbarActionsBarBubbleDelegate::CLOSE_LEARN_MORE;
  } else if (dismissButton_ && sender == dismissButton_) {
    action = ToolbarActionsBarBubbleDelegate::CLOSE_DISMISS_USER_ACTION;
  } else {
    DCHECK_EQ(sender, actionButton_);
    action = ToolbarActionsBarBubbleDelegate::CLOSE_EXECUTE;
  }
  acknowledged_ = YES;
  delegate_->OnBubbleClosed(action);
  [self close];
}

@end
