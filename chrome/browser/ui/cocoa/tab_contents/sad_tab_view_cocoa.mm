// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/tab_contents/sad_tab_view_cocoa.h"

#include <vector>

#import "base/mac/foundation_util.h"
#include "components/grit/components_scaled_resources.h"
#import "ui/base/cocoa/controls/blue_label_button.h"
#import "ui/base/cocoa/controls/hyperlink_text_view.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

// Maximum width used by page contents.
const CGFloat kMaxContainerWidth = 600;
// Padding between icon and title.
const CGFloat kIconTitleSpacing = 40;
// Space between lines of the title.
const CGFloat kTitleLineSpacing = 6;
// Padding between title and message.
const CGFloat kTitleMessageSpacing = 18;
// Padding between message and link.
const CGFloat kMessageLinkSpacing = 50;
// Padding between message and button.
const CGFloat kMessageButtonSpacing = 44;
// Minimum margins on all sides.
const CGFloat kTabMargin = 13;
// Maximum margin on top.
const CGFloat kMaxTopMargin = 130;

}  // namespace

@interface SadTabContainerView : NSView
@end

@implementation SadTabContainerView
- (BOOL)isFlipped {
  return YES;
}
@end

@interface SadTabView ()<NSTextViewDelegate>
@end

@implementation SadTabView {
  NSView* container_;
  NSTextView* message_;
  HyperlinkTextView* help_;
  NSButton* button_;
  SadTab* sadTab_;
  BOOL recordedFirstPaint_;
}

- (instancetype)initWithFrame:(NSRect)frame sadTab:(SadTab*)sadTab {
  if ((self = [super initWithFrame:frame])) {
    sadTab_ = sadTab;
    recordedFirstPaint_ = NO;

    self.wantsLayer = YES;
    self.layer.backgroundColor =
        [NSColor colorWithCalibratedWhite:245.0f / 255.0f alpha:1.0].CGColor;
    container_ = [[SadTabContainerView new] autorelease];

    NSImage* iconImage = ui::ResourceBundle::GetSharedInstance()
                             .GetNativeImageNamed(IDR_CRASH_SAD_TAB)
                             .ToNSImage();
    NSImageView* icon = [[NSImageView new] autorelease];
    icon.image = iconImage;
    icon.frameSize = iconImage.size;
    [container_ addSubview:icon];

    message_ = [[[NSTextView alloc]
        initWithFrame:NSMakeRect(0, NSMaxY(icon.frame) + kIconTitleSpacing,
                                 NSWidth(container_.bounds), 0)] autorelease];
    message_.editable = NO;
    message_.drawsBackground = NO;
    message_.autoresizingMask = NSViewWidthSizable;

    NSMutableParagraphStyle* titleParagraphStyle =
        [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];
    titleParagraphStyle.lineSpacing = kTitleLineSpacing;
    titleParagraphStyle.paragraphSpacing =
        kTitleMessageSpacing - kTitleLineSpacing;
    [message_.textStorage
        appendAttributedString:
            [[[NSAttributedString alloc]
                initWithString:[NSString
                                   stringWithFormat:@"%@\n",
                                                    l10n_util::GetNSString(
                                                        sadTab->GetTitle())]
                    attributes:@{
                      NSParagraphStyleAttributeName : titleParagraphStyle,
                      NSFontAttributeName : [NSFont systemFontOfSize:24],
                      NSForegroundColorAttributeName :
                          [NSColor colorWithCalibratedWhite:38.0f / 255.0f
                                                      alpha:1.0],
                    }] autorelease]];

    NSFont* messageFont = [NSFont systemFontOfSize:14];
    NSColor* messageColor =
        [NSColor colorWithCalibratedWhite:81.0f / 255.0f alpha:1.0];
    [message_.textStorage
        appendAttributedString:[[[NSAttributedString alloc]
                                   initWithString:l10n_util::GetNSString(
                                                      sadTab->GetMessage())
                                       attributes:@{
                                         NSFontAttributeName : messageFont,
                                         NSForegroundColorAttributeName :
                                             messageColor,
                                       }] autorelease]];
    std::vector<int> subMessages = sadTab->GetSubMessages();
    if (!subMessages.empty()) {
      NSTextList* textList =
          [[[NSTextList alloc] initWithMarkerFormat:@"{disc}" options:0]
              autorelease];
      NSMutableParagraphStyle* paragraphStyle =
          [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];
      paragraphStyle.textLists = @[ textList ];
      paragraphStyle.paragraphSpacingBefore = messageFont.capHeight;
      paragraphStyle.headIndent =
          static_cast<NSTextTab*>(paragraphStyle.tabStops[1]).location;

      NSMutableString* subMessageString = [NSMutableString string];
      for (int subMessage : subMessages) {
        // All markers are disc so pass 0 here for item number.
        [subMessageString appendFormat:@"\n\t%@\t%@",
                                       [textList markerForItemNumber:0],
                                       l10n_util::GetNSString(subMessage)];
      }
      [message_.textStorage
          appendAttributedString:[[[NSAttributedString alloc]
                                     initWithString:subMessageString
                                         attributes:@{
                                           NSParagraphStyleAttributeName :
                                               paragraphStyle,
                                           NSFontAttributeName : messageFont,
                                           NSForegroundColorAttributeName :
                                               messageColor,
                                         }] autorelease]];
    }
    [message_ sizeToFit];
    [container_ addSubview:message_];

    NSString* helpLinkTitle =
        l10n_util::GetNSString(sadTab->GetHelpLinkTitle());
    help_ = [[[HyperlinkTextView alloc]
        initWithFrame:NSMakeRect(0, 0, 1, message_.font.pointSize + 4)]
        autorelease];
    help_.delegate = self;
    help_.autoresizingMask = NSViewWidthSizable;
    help_.textContainer.lineFragmentPadding = 2;  // To align with message_.
    [help_ setMessage:helpLinkTitle
             withFont:messageFont
         messageColor:messageColor];
    [help_ addLinkRange:NSMakeRange(0, helpLinkTitle.length)
                withURL:@(sadTab->GetHelpLinkURL())
              linkColor:messageColor];
    [help_ sizeToFit];
    [container_ addSubview:help_];

    button_ = [[BlueLabelButton new] autorelease];
    button_.target = self;
    button_.action = @selector(buttonClicked);
    button_.title = l10n_util::GetNSString(sadTab->GetButtonTitle());
    [button_ sizeToFit];
    [container_ addSubview:button_];

    [self addSubview:container_];
    [self resizeSubviewsWithOldSize:self.bounds.size];
  }
  return self;
}

- (BOOL)isOpaque {
  return YES;
}

- (BOOL)isFlipped {
  return YES;
}

- (void)updateLayer {
  // updateLayer seems to be called whenever NSBackingLayerDisplayIfNeeded is
  // called by AppKit, which could be multiple times - at least twice has been
  // observed. Guard against repeated recordings of first paint.
  if (!recordedFirstPaint_) {
    sadTab_->RecordFirstPaint();
    recordedFirstPaint_ = YES;
  }
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize {
  [super resizeSubviewsWithOldSize:oldSize];

  NSSize size = self.bounds.size;
  NSSize containerSize = NSMakeSize(
      std::min(size.width - 2 * kTabMargin, kMaxContainerWidth), size.height);

  // Set the container's size first because text wrapping depends on its width.
  container_.frameSize = containerSize;

  help_.frameOrigin =
      NSMakePoint(0, NSMaxY(message_.frame) + kMessageLinkSpacing);

  button_.frameOrigin =
      NSMakePoint(containerSize.width - NSWidth(button_.bounds),
                  NSMaxY(message_.frame) + kMessageButtonSpacing);

  containerSize.height = NSMaxY(button_.frame);
  container_.frameSize = containerSize;

  // Center. Top margin is must be between kTabMargin and kMaxTopMargin.
  container_.frameOrigin = NSMakePoint(
      floor((size.width - containerSize.width) / 2),
      std::min(kMaxTopMargin,
               std::max(kTabMargin,
                        size.height - containerSize.height - kTabMargin)));
}

- (void)buttonClicked {
  sadTab_->PerformAction(SadTab::Action::BUTTON);
}

// Called when someone clicks on the embedded link.
- (BOOL)textView:(NSTextView*)textView
    clickedOnLink:(id)link
          atIndex:(NSUInteger)charIndex {
  sadTab_->PerformAction(SadTab::Action::HELP_LINK);
  return YES;
}

@end
