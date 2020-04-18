// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/browser/zoom_bubble_controller.h"

#include "base/i18n/number_formatting.h"
#include "base/mac/foundation_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#include "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#import "chrome/browser/ui/cocoa/location_bar/zoom_decoration.h"
#include "chrome/grit/generated_resources.h"
#include "components/zoom/page_zoom.h"
#include "components/zoom/zoom_controller.h"
#include "content/public/common/page_zoom.h"
#include "skia/ext/skia_utils_mac.h"
#import "ui/base/cocoa/hover_button.h"
#import "ui/base/cocoa/window_size_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/native_theme/native_theme.h"

@interface ZoomBubbleController (Private)
- (void)performLayout;
- (void)autoCloseBubble;
- (NSAttributedString*)attributedStringWithString:(NSString*)string
                                         fontSize:(CGFloat)fontSize;
// Adds a new zoom button to the bubble.
- (NSButton*)addButtonWithTitleID:(int)titleID
                         fontSize:(CGFloat)fontSize
                           action:(SEL)action;
- (NSTextField*)addZoomPercentTextField;
- (void)updateAutoCloseTimer;

// Get the WebContents instance and apply the indicated zoom.
- (void)zoomHelper:(content::PageZoom)alterPageZoom;
@end

// Button that highlights the background on mouse over.
@interface ZoomHoverButton : HoverButton
@end

namespace {

// The amount of time to wait before the bubble automatically closes.
// Should keep in sync with kBubbleCloseDelay in
// src/chrome/browser/ui/views/location_bar/zoom_bubble_view.cc.
NSTimeInterval gAutoCloseDelay = 1.5;

// The height of the window.
const CGFloat kWindowHeight = 29.0;

// Width of the zoom in and zoom out buttons.
const CGFloat kZoomInOutButtonWidth = 44.0;

// Width of zoom label.
const CGFloat kZoomLabelWidth = 55.0;

// Horizontal margin for the reset zoom button.
const CGFloat kResetZoomMargin = 9.0;

// The font size text shown in the bubble.
const CGFloat kTextFontSize = 12.0;

// The font size of the zoom in and zoom out buttons.
const CGFloat kZoomInOutButtonFontSize = 16.0;

}  // namespace

namespace chrome {

void SetZoomBubbleAutoCloseDelayForTesting(NSTimeInterval time_interval) {
  gAutoCloseDelay = time_interval;
}

}  // namespace chrome

@implementation ZoomBubbleController

@synthesize delegate = delegate_;

- (id)initWithParentWindow:(NSWindow*)parentWindow
                  delegate:(ZoomBubbleControllerDelegate*)delegate {
  base::scoped_nsobject<InfoBubbleWindow> window(
      [[InfoBubbleWindow alloc] initWithContentRect:NSMakeRect(0, 0, 200, 100)
                                          styleMask:NSBorderlessWindowMask
                                            backing:NSBackingStoreBuffered
                                              defer:NO]);
  if ((self = [super initWithWindow:window
                       parentWindow:parentWindow
                         anchoredAt:NSZeroPoint])) {
    [window setInfoBubbleCanBecomeKeyWindow:NO];
    delegate_ = delegate;

    ui::NativeTheme* nativeTheme = ui::NativeTheme::GetInstanceForNativeUi();
    [[self bubble] setAlignment:info_bubble::kAlignTrailingEdgeToAnchorEdge];
    [[self bubble] setArrowLocation:info_bubble::kNoArrow];
    [[self bubble] setBackgroundColor:
        skia::SkColorToCalibratedNSColor(nativeTheme->GetSystemColor(
            ui::NativeTheme::kColorId_DialogBackground))];

    [self performLayout];

    trackingArea_.reset([[CrTrackingArea alloc]
        initWithRect:NSZeroRect
             options:NSTrackingMouseEnteredAndExited |
                     NSTrackingActiveAlways |
                     NSTrackingInVisibleRect
               owner:self
            userInfo:nil]);
    [trackingArea_.get() clearOwnerWhenWindowWillClose:[self window]];
    [[[self window] contentView] addTrackingArea:trackingArea_.get()];
  }
  return self;
}

- (void)showAnchoredAt:(NSPoint)anchorPoint autoClose:(BOOL)autoClose {
  [self onZoomChanged];
  InfoBubbleWindow* window =
      base::mac::ObjCCastStrict<InfoBubbleWindow>([self window]);
  [window setAllowedAnimations:autoClose
      ? info_bubble::kAnimateOrderIn | info_bubble::kAnimateOrderOut
      : info_bubble::kAnimateNone];

  self.anchorPoint = anchorPoint;
  [self showWindow:nil];

  autoClose_ = autoClose;
  [self updateAutoCloseTimer];
}

- (void)onZoomChanged {
  // |delegate_| may be set null by this object's owner.
  if (!delegate_)
    return;

  // TODO(shess): It may be appropriate to close the window if
  // |contents| or |zoomController| are NULL.  But they can be NULL in
  // tests.

  content::WebContents* contents = delegate_->GetWebContents();
  if (!contents)
    return;

  zoom::ZoomController* zoomController =
      zoom::ZoomController::FromWebContents(contents);
  if (!zoomController)
    return;

  int percent = zoomController->GetZoomPercent();
  NSString* string = base::SysUTF16ToNSString(base::FormatPercent(percent));
  [zoomPercent_ setAttributedStringValue:
      [self attributedStringWithString:string
                              fontSize:kTextFontSize]];

  [self updateAutoCloseTimer];
}

- (void)resetToDefault:(id)sender {
  [self zoomHelper:content::PAGE_ZOOM_RESET];
}

- (void)zoomIn:(id)sender {
  [self zoomHelper:content::PAGE_ZOOM_IN];
}

- (void)zoomOut:(id)sender {
  [self zoomHelper:content::PAGE_ZOOM_OUT];
}

- (void)closeWithoutAnimation {
  InfoBubbleWindow* window =
      base::mac::ObjCCastStrict<InfoBubbleWindow>([self window]);
  [window setAllowedAnimations:info_bubble::kAnimateNone];
  [self close];
}

// OmniboxDecorationBubbleController implementation.
- (LocationBarDecoration*)decorationForBubble {
  BrowserWindowController* controller = [BrowserWindowController
      browserWindowControllerForWindow:[self parentWindow]];
  LocationBarViewMac* locationBar = [controller locationBarBridge];
  return locationBar ? locationBar->zoom_decoration() : nullptr;
}

// NSWindowController implementation.
- (void)windowWillClose:(NSNotification*)notification {
  // |delegate_| may be set null by this object's owner.
  if (delegate_) {
    delegate_->OnClose();
    delegate_ = NULL;
  }
  [NSObject cancelPreviousPerformRequestsWithTarget:self
                                           selector:@selector(autoCloseBubble)
                                             object:nil];
  [super windowWillClose:notification];
}

- (void)mouseEntered:(NSEvent*)theEvent {
  isMouseInside_ = YES;
  [self updateAutoCloseTimer];
}

- (void)mouseExited:(NSEvent*)theEvent {
  isMouseInside_ = NO;
  [self updateAutoCloseTimer];
}

// Private /////////////////////////////////////////////////////////////////////

- (void)performLayout {
  // Zoom out button.
  NSButton* zoomOutButton = [self addButtonWithTitleID:IDS_ZOOM_MINUS2
                                              fontSize:kZoomInOutButtonFontSize
                                                action:@selector(zoomOut:)];
  NSRect rect = NSMakeRect(0, 0, kZoomInOutButtonWidth, kWindowHeight);
  [zoomOutButton setFrame:rect];

  // Zoom label.
  zoomPercent_.reset([[self addZoomPercentTextField] retain]);
  rect.origin.x += NSWidth(rect);
  rect.size.width = kZoomLabelWidth;
  [zoomPercent_ sizeToFit];
  NSRect zoomRect = rect;
  zoomRect.size.height = NSHeight([zoomPercent_ frame]);
  zoomRect.origin.y = roundf((NSHeight(rect) - NSHeight(zoomRect)) / 2.0);
  [zoomPercent_ setFrame:zoomRect];

  // Zoom in button.
  NSButton* zoomInButton = [self addButtonWithTitleID:IDS_ZOOM_PLUS2
                                             fontSize:kZoomInOutButtonFontSize
                                               action:@selector(zoomIn:)];
  rect.origin.x += NSWidth(rect);
  rect.size.width = kZoomInOutButtonWidth;
  [zoomInButton setFrame:rect];

  // Separator view.
  rect.origin.x += NSWidth(rect);
  rect.size.width = 1;
  base::scoped_nsobject<NSBox> separatorView(
      [[NSBox alloc] initWithFrame:rect]);
  [separatorView setBoxType:NSBoxCustom];
  ui::NativeTheme* nativeTheme = ui::NativeTheme::GetInstanceForNativeUi();
  [separatorView setBorderColor:
      skia::SkColorToCalibratedNSColor(nativeTheme->GetSystemColor(
          ui::NativeTheme::kColorId_MenuSeparatorColor))];
  [[[self window] contentView] addSubview:separatorView];

  // Reset zoom button.
  NSButton* resetButton =
      [self addButtonWithTitleID:IDS_ZOOM_SET_DEFAULT
                        fontSize:kTextFontSize
                          action:@selector(resetToDefault:)];
  rect.origin.x += NSWidth(rect);
  rect.size.width =
      [[resetButton attributedTitle] size].width + kResetZoomMargin * 2.0;
  [resetButton setFrame:rect];

  // Update window frame.
  NSRect windowFrame = [[self window] frame];
  windowFrame.size.height = NSHeight(rect);
  windowFrame.size.width = NSMaxX(rect);
  [[self window] setFrame:windowFrame display:YES];
}

- (void)autoCloseBubble {
  if (!autoClose_)
    return;
  [self close];
}

- (NSAttributedString*)attributedStringWithString:(NSString*)string
                                           fontSize:(CGFloat)fontSize {
  base::scoped_nsobject<NSMutableParagraphStyle> paragraphStyle(
      [[NSMutableParagraphStyle alloc] init]);
  [paragraphStyle setAlignment:NSCenterTextAlignment];
  NSDictionary* attributes = @{
      NSFontAttributeName:
      [NSFont systemFontOfSize:fontSize],
      NSForegroundColorAttributeName:
      [NSColor colorWithCalibratedWhite:0.58 alpha:1.0],
      NSParagraphStyleAttributeName:
      paragraphStyle.get()
  };
  return [[[NSAttributedString alloc]
      initWithString:string
          attributes:attributes] autorelease];
}

- (NSButton*)addButtonWithTitleID:(int)titleID
                         fontSize:(CGFloat)fontSize
                           action:(SEL)action {
  base::scoped_nsobject<NSButton> button(
      [[ZoomHoverButton alloc] initWithFrame:NSZeroRect]);
  NSString* title = l10n_util::GetNSStringWithFixup(titleID);
  [button setAttributedTitle:[self attributedStringWithString:title
                                                     fontSize:fontSize]];
  [[button cell] setBordered:NO];
  [button setTarget:self];
  [button setAction:action];
  [[[self window] contentView] addSubview:button];
  return button.autorelease();
}

- (NSTextField*)addZoomPercentTextField {
  base::scoped_nsobject<NSTextField> textField(
      [[NSTextField alloc] initWithFrame:NSZeroRect]);
  [textField setEditable:NO];
  [textField setBordered:NO];
  [textField setDrawsBackground:NO];
  [[[self window] contentView] addSubview:textField];
  return textField.autorelease();
}

- (void)updateAutoCloseTimer {
  [NSObject cancelPreviousPerformRequestsWithTarget:self
                                           selector:@selector(autoCloseBubble)
                                             object:nil];
  if (autoClose_ && !isMouseInside_) {
    [self performSelector:@selector(autoCloseBubble)
               withObject:nil
               afterDelay:gAutoCloseDelay];
  }
}

- (void)zoomHelper:(content::PageZoom)alterPageZoom {
  // |delegate| can be null after -windowWillClose:.
  if (!delegate_)
    return;
  content::WebContents* webContents = delegate_->GetWebContents();

  // TODO(shess): Zoom() immediately dereferences |webContents|, and
  // there haven't been associated crashes in the wild, so it seems
  // fine in practice.  It might make sense to close the bubble in
  // that case, though.
  zoom::PageZoom::Zoom(webContents, alterPageZoom);
}

@end

@implementation ZoomHoverButton

- (void)drawRect:(NSRect)rect {
  NSRect bounds = [self bounds];
  NSAttributedString* title = [self attributedTitle];
  if ([self hoverState] != kHoverStateNone) {
    ui::NativeTheme* nativeTheme = ui::NativeTheme::GetInstanceForNativeUi();
    [skia::SkColorToCalibratedNSColor(nativeTheme->GetSystemColor(
        ui::NativeTheme::kColorId_FocusedMenuItemBackgroundColor)) set];
    NSRectFillUsingOperation(bounds, NSCompositeSourceOver);

    // Change the title color.
    base::scoped_nsobject<NSMutableAttributedString> selectedTitle(
        [[NSMutableAttributedString alloc] initWithAttributedString:title]);
    NSColor* selectedTitleColor =
        skia::SkColorToCalibratedNSColor(nativeTheme->GetSystemColor(
            ui::NativeTheme::kColorId_SelectedMenuItemForegroundColor));
    [selectedTitle addAttribute:NSForegroundColorAttributeName
                          value:selectedTitleColor
                          range:NSMakeRange(0, [title length])];
    title = selectedTitle.autorelease();
  }

  [[self cell] drawTitle:title
               withFrame:bounds
                  inView:self];
}

@end
