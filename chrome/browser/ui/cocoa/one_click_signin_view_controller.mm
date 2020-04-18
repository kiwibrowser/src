// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/one_click_signin_view_controller.h"

#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#import "chrome/browser/ui/cocoa/chrome_style.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_contents.h"
#include "skia/ext/skia_utils_mac.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#import "ui/base/cocoa/controls/hyperlink_text_view.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace {

// The margin between the top edge of the border and the error message text in
// the sign-in bubble, in the case of an error.
const CGFloat kTopErrorMessageMargin = 12;

// Shift the origin of |view|'s frame by the given amount in the
// positive y direction (up).
void ShiftOriginY(NSView* view, CGFloat amount) {
  NSPoint origin = [view frame].origin;
  origin.y += amount;
  [view setFrameOrigin:origin];
}

}  // namespace

@interface OneClickSigninViewController ()
- (CGFloat)initializeInformativeTextView;
- (void)close;
@end

@implementation OneClickSigninViewController

- (id)initWithNibName:(NSString*)nibName
          webContents:(content::WebContents*)webContents
         syncCallback:(const BrowserWindow::StartSyncCallback&)syncCallback
        closeCallback:(const base::Closure&)closeCallback
         isSyncDialog:(BOOL)isSyncDialog
                email:(const base::string16&)email
         errorMessage:(NSString*)errorMessage {
  if ((self = [super initWithNibName:nibName
                              bundle:base::mac::FrameworkBundle()])) {
    webContents_ = webContents;
    startSyncCallback_ = syncCallback;
    closeCallback_ = closeCallback;
    isSyncDialog_ = isSyncDialog;
    clickedLearnMore_ = NO;
    email_ = email;
    errorMessage_.reset([errorMessage retain]);
    if (isSyncDialog_)
      DCHECK(!startSyncCallback_.is_null());
  }
  return self;
}

- (void)viewWillClose {
  // This is usually called after a click handler has initiated sync
  // and has reset the callback. However, in the case that we are closing
  // the window and nothing else has initiated the sync, we must do so here
  if (isSyncDialog_ && !startSyncCallback_.is_null()) {
    base::ResetAndReturn(&startSyncCallback_).Run(
        OneClickSigninSyncStarter::UNDO_SYNC);
  }
}

- (IBAction)ok:(id)sender {
  if (isSyncDialog_) {
    signin_metrics::LogSigninConfirmHistogramValue(
        clickedLearnMore_ ?
            signin_metrics::HISTOGRAM_CONFIRM_LEARN_MORE_OK :
            signin_metrics::HISTOGRAM_CONFIRM_OK);

    base::ResetAndReturn(&startSyncCallback_).Run(
      OneClickSigninSyncStarter::SYNC_WITH_DEFAULT_SETTINGS);
  }
  [self close];
}

- (IBAction)onClickUndo:(id)sender {
  if (isSyncDialog_) {
    signin_metrics::LogSigninConfirmHistogramValue(
        clickedLearnMore_ ?
            signin_metrics::HISTOGRAM_CONFIRM_LEARN_MORE_UNDO :
            signin_metrics::HISTOGRAM_CONFIRM_UNDO);

    base::ResetAndReturn(&startSyncCallback_).Run(
      OneClickSigninSyncStarter::UNDO_SYNC);
  }
  [self close];
}

- (IBAction)onClickAdvancedLink:(id)sender {
  if (isSyncDialog_) {
    signin_metrics::LogSigninConfirmHistogramValue(
        clickedLearnMore_ ?
            signin_metrics::HISTOGRAM_CONFIRM_LEARN_MORE_ADVANCED :
            signin_metrics::HISTOGRAM_CONFIRM_ADVANCED);

    base::ResetAndReturn(&startSyncCallback_).Run(
        OneClickSigninSyncStarter::CONFIGURE_SYNC_FIRST);
  }
  else {
    content::OpenURLParams params(
        GURL(chrome::kChromeUISettingsURL), content::Referrer(),
        WindowOpenDisposition::CURRENT_TAB, ui::PAGE_TRANSITION_LINK, false);
    webContents_->OpenURL(params);
  }
  [self close];
}

- (IBAction)onClickClose:(id)sender {
  if (isSyncDialog_) {
    signin_metrics::LogSigninConfirmHistogramValue(
        clickedLearnMore_ ?
            signin_metrics::HISTOGRAM_CONFIRM_LEARN_MORE_CLOSE :
            signin_metrics::HISTOGRAM_CONFIRM_CLOSE);

    base::ResetAndReturn(&startSyncCallback_).Run(
        OneClickSigninSyncStarter::UNDO_SYNC);
  }
  [self close];
}

- (void)awakeFromNib {
  // Lay out the text controls from the bottom up.
  CGFloat totalYOffset = 0.0;

  if ([errorMessage_ length] == 0) {
    totalYOffset +=
        [GTMUILocalizerAndLayoutTweaker sizeToFitView:advancedLink_].height;
    [[advancedLink_ cell] setTextColor:
        skia::SkColorToCalibratedNSColor(chrome_style::GetLinkColor())];
  } else {
    // Don't display the advanced link for the error bubble.
    // To align the Learn More link with the OK button, we need to offset by
    // the height of the Advanced link, plus the padding between it and the
    // Learn More link above.
    float advancedLinkHeightPlusPadding =
        [informativePlaceholderTextField_ frame].origin.y -
        [advancedLink_ frame].origin.y;

    totalYOffset -= advancedLinkHeightPlusPadding;
    [advancedLink_ removeFromSuperview];
  }

  if (informativePlaceholderTextField_) {
    if (!isSyncDialog_ && ([errorMessage_ length] != 0)) {
      // Move up the "Learn more" origin in error case to account for the
      // smaller bubble.
      NSRect frame = [informativePlaceholderTextField_ frame];
      frame = NSOffsetRect(frame, 0, NSHeight([titleTextField_ frame]));
      [informativePlaceholderTextField_ setFrame:frame];
    }

    ShiftOriginY(informativePlaceholderTextField_, totalYOffset);
    totalYOffset += [self initializeInformativeTextView];
  }

  ShiftOriginY(messageTextField_, totalYOffset);
  totalYOffset +=
      [GTMUILocalizerAndLayoutTweaker
          sizeToFitFixedWidthTextField:messageTextField_];

  ShiftOriginY(titleTextField_, totalYOffset);
  totalYOffset +=
      [GTMUILocalizerAndLayoutTweaker
          sizeToFitFixedWidthTextField:titleTextField_];

  NSSize delta = NSMakeSize(0.0, totalYOffset);

  if (isSyncDialog_) {
    [messageTextField_ setStringValue:l10n_util::GetNSStringWithFixup(
        IDS_ONE_CLICK_SIGNIN_DIALOG_TITLE_NEW)];
  } else if ([errorMessage_ length] != 0) {
    [titleTextField_ setHidden:YES];
    [messageTextField_ setStringValue:errorMessage_];

    // Make the bubble less tall, as the title text will be hidden.
    NSSize size = [[self view] frame].size;
    size.height = size.height - NSHeight([titleTextField_ frame]);
    [[self view] setFrameSize:size];

    // Shift the message text up to where the title text used to be.
    NSPoint origin = [titleTextField_ frame].origin;
    [messageTextField_ setFrameOrigin:origin];
    ShiftOriginY(messageTextField_, -kTopErrorMessageMargin);

    // Use "OK" instead of "OK, got it" in the error case, and size the button
    // accordingly.
    [closeButton_ setTitle:l10n_util::GetNSStringWithFixup(
        IDS_OK)];
    [GTMUILocalizerAndLayoutTweaker sizeToFitView:[closeButton_ superview]];
  }

  // Resize bubble and window to hold the controls.
  [GTMUILocalizerAndLayoutTweaker
      resizeViewWithoutAutoResizingSubViews:[self view]
                                      delta:delta];

  if (isSyncDialog_) {
    signin_metrics::LogSigninConfirmHistogramValue(
        signin_metrics::HISTOGRAM_CONFIRM_SHOWN);
  }
}

- (CGFloat)initializeInformativeTextView {
  NSRect oldFrame = [informativePlaceholderTextField_ frame];

  // Replace the placeholder NSTextField with the real label NSTextView. The
  // former doesn't show links in a nice way, but the latter can't be added in
  // a xib without a containing scroll view, so create the NSTextView
  // programmatically.
  informativeTextView_.reset(
      [[HyperlinkTextView alloc] initWithFrame:oldFrame]);
  [informativeTextView_.get() setAutoresizingMask:
      [informativePlaceholderTextField_ autoresizingMask]];
  [informativeTextView_.get() setDelegate:self];

  // Set the text.
  NSString* learnMoreText = l10n_util::GetNSStringWithFixup(IDS_LEARN_MORE);
  NSString* messageText;
  NSUInteger learnMoreOffset = 0;

  ui::ResourceBundle::FontStyle fontStyle = isSyncDialog_ ?
      chrome_style::kTextFontStyle : ui::ResourceBundle::SmallFont;
  NSFont* font = ui::ResourceBundle::GetSharedInstance().GetFont(
      fontStyle).GetNativeFont();

  // The non-modal bubble already has a text content and only needs the
  // Learn More link (in a smaller font).
  if (isSyncDialog_) {
    messageText = l10n_util::GetNSStringFWithFixup(
        IDS_ONE_CLICK_SIGNIN_DIALOG_MESSAGE_NEW, email_);
    learnMoreOffset = [messageText length];
    messageText = [messageText stringByAppendingFormat:@" %@", learnMoreText];
  } else {
    messageText = learnMoreText;
  }

  NSColor* linkColor =
      skia::SkColorToCalibratedNSColor(chrome_style::GetLinkColor());
  [informativeTextView_ setMessage:messageText
                          withFont:font
                      messageColor:[NSColor blackColor]];
  [informativeTextView_ addLinkRange:NSMakeRange(learnMoreOffset,
                                                 [learnMoreText length])
                             withURL:@(chrome::kChromeSyncLearnMoreURL)
                           linkColor:linkColor];

  // Make the "Advanced" link font as large as the "Learn More" link.
  [[advancedLink_ cell] setFont:font];
  [advancedLink_ sizeToFit];

  // Size to fit.
  [[informativePlaceholderTextField_ cell] setAttributedStringValue:
      [informativeTextView_ attributedString]];
  [GTMUILocalizerAndLayoutTweaker
        sizeToFitFixedWidthTextField:informativePlaceholderTextField_];
  NSRect newFrame = [informativePlaceholderTextField_ frame];
  [informativeTextView_ setFrame:newFrame];

  // Swap placeholder.
  [[informativePlaceholderTextField_ superview]
     replaceSubview:informativePlaceholderTextField_
               with:informativeTextView_.get()];
  informativePlaceholderTextField_ = nil;  // Now released.

  return NSHeight(newFrame) - NSHeight(oldFrame);
}

- (BOOL)textView:(NSTextView*)textView
   clickedOnLink:(id)link
         atIndex:(NSUInteger)charIndex {
  if (isSyncDialog_ && !clickedLearnMore_) {
    clickedLearnMore_ = YES;

    signin_metrics::LogSigninConfirmHistogramValue(
        signin_metrics::HISTOGRAM_CONFIRM_LEARN_MORE);
  }
  WindowOpenDisposition location =
      isSyncDialog_ ? WindowOpenDisposition::NEW_WINDOW
                    : WindowOpenDisposition::NEW_FOREGROUND_TAB;
  content::OpenURLParams params(GURL(chrome::kChromeSyncLearnMoreURL),
                                content::Referrer(), location,
                                ui::PAGE_TRANSITION_LINK, false);
  webContents_->OpenURL(params);
  return YES;
}

- (void)close {
  base::ResetAndReturn(&closeCallback_).Run();
}

@end

@implementation OneClickSigninViewController (TestingAPI)

- (NSTextView*)linkViewForTesting {
  return informativeTextView_.get();
}

@end
