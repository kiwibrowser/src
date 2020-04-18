// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/screen_capture_notification_ui_cocoa.h"

#import <Cocoa/Cocoa.h>

#include "base/compiler_specific.h"
#include "base/i18n/rtl.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "skia/ext/skia_utils_mac.h"
#import "ui/base/cocoa/controls/blue_label_button.h"
#import "ui/base/cocoa/controls/hyperlink_button_cell.h"
#include "ui/base/cocoa/window_size_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_features.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/mac/nswindow_frame_controls.h"
#include "ui/gfx/text_elider.h"
#include "ui/native_theme/native_theme.h"

const CGFloat kMinimumWidth = 460;
const CGFloat kMaximumWidth = 1000;
const CGFloat kHorizontalMargin = 10;
const CGFloat kPaddingVertical = 5;
const CGFloat kPaddingHorizontal = 10;
const CGFloat kWindowCornerRadius = 2;
const CGFloat kWindowAlphaValue = 0.85;

@interface ScreenCaptureNotificationController()
- (void)hide;
- (void)populateWithText:(const base::string16&)text;
@end

@interface ScreenCaptureNotificationView : NSView
@end

@interface WindowGripView : NSImageView
- (WindowGripView*)init;
@end


ScreenCaptureNotificationUICocoa::ScreenCaptureNotificationUICocoa(
    const base::string16& text)
    : text_(text) {
}

ScreenCaptureNotificationUICocoa::~ScreenCaptureNotificationUICocoa() {}

gfx::NativeViewId ScreenCaptureNotificationUICocoa::OnStarted(
    const base::Closure& stop_callback) {
  DCHECK(!stop_callback.is_null());
  DCHECK(!windowController_);

  windowController_.reset([[ScreenCaptureNotificationController alloc]
      initWithCallback:stop_callback
                  text:text_]);
  [windowController_ showWindow:nil];
  return [[windowController_ window] windowNumber];
}

std::unique_ptr<ScreenCaptureNotificationUI>
ScreenCaptureNotificationUI::CreateCocoa(const base::string16& text) {
  if (chrome::ShowAllDialogsWithViewsToolkit())
    return nullptr;

  return std::unique_ptr<ScreenCaptureNotificationUI>(
      new ScreenCaptureNotificationUICocoa(text));
}

@implementation ScreenCaptureNotificationController
- (id)initWithCallback:(const base::Closure&)stop_callback
                  text:(const base::string16&)text {
  base::scoped_nsobject<NSWindow> window(
      [[NSWindow alloc] initWithContentRect:ui::kWindowSizeDeterminedLater
                                  styleMask:NSBorderlessWindowMask
                                    backing:NSBackingStoreBuffered
                                      defer:NO]);
  [window setReleasedWhenClosed:NO];
  [window setAlphaValue:kWindowAlphaValue];
  [window setBackgroundColor:[NSColor clearColor]];
  [window setOpaque:NO];
  [window setHasShadow:YES];
  [window setLevel:NSStatusWindowLevel];
  [window setMovableByWindowBackground:YES];
  [window setDelegate:self];
  gfx::SetNSWindowVisibleOnAllWorkspaces(window, true);

  self = [super initWithWindow:window];
  if (self) {
    stop_callback_ = stop_callback;
    [self populateWithText:text];

    // Center the window at the bottom of the main screen, above the dock (if
    // present).
    NSRect desktopRect = [[NSScreen mainScreen] visibleFrame];
    NSRect contentRect = [[window contentView] frame];
    NSRect windowRect = NSMakeRect(
        (NSWidth(desktopRect) - NSWidth(contentRect)) / 2 + NSMinX(desktopRect),
        NSMinY(desktopRect), NSWidth(contentRect), NSHeight(contentRect));
    [window setFrame:windowRect display:YES];
  }
  return self;
}

- (void)dealloc {
  [stopButton_ setTarget:nil];
  [minimizeButton_ setTarget:nil];
  [super dealloc];
}

- (void)stopSharing:(id)sender {
  if (!stop_callback_.is_null()) {
    base::Closure callback = stop_callback_;
    stop_callback_.Reset();
    callback.Run();  // Deletes |self|.
  }
}

- (void)minimize:(id)sender {
  [[self window] miniaturize:sender];
}

- (void)hide {
  stop_callback_.Reset();
  [self close];
}

- (void)populateWithText:(const base::string16&)text {
  base::scoped_nsobject<ScreenCaptureNotificationView> content(
      [[ScreenCaptureNotificationView alloc]
          initWithFrame:ui::kWindowSizeDeterminedLater]);
  [[self window] setContentView:content];

  // Create button.
  stopButton_.reset([[BlueLabelButton alloc] initWithFrame:NSZeroRect]);
  [stopButton_ setTitle:l10n_util::GetNSString(
                  IDS_MEDIA_SCREEN_CAPTURE_NOTIFICATION_STOP)];
  [stopButton_ setTarget:self];
  [stopButton_ setAction:@selector(stopSharing:)];
  [stopButton_ sizeToFit];
  [content addSubview:stopButton_];

  base::scoped_nsobject<HyperlinkButtonCell> cell(
      [[HyperlinkButtonCell alloc]
       initTextCell:l10n_util::GetNSString(
                        IDS_PASSWORDS_PAGE_VIEW_HIDE_BUTTON)]);

  minimizeButton_.reset([[NSButton alloc] initWithFrame:NSZeroRect]);
  [minimizeButton_ setCell:cell.get()];
  [minimizeButton_ sizeToFit];
  [minimizeButton_ setTarget:self];
  [minimizeButton_ setAction:@selector(minimize:)];
  [content addSubview:minimizeButton_];

  CGFloat buttonsWidth = NSWidth([stopButton_ frame]) + kHorizontalMargin +
      NSWidth([minimizeButton_ frame]);
  CGFloat totalHeight =
      kPaddingVertical + NSHeight([stopButton_ frame]) + kPaddingVertical;

  // Create grip icon.
  base::scoped_nsobject<WindowGripView> gripView([[WindowGripView alloc] init]);
  [content addSubview:gripView];
  CGFloat gripWidth = NSWidth([gripView frame]);
  CGFloat gripHeight = NSHeight([gripView frame]);
  [gripView setFrameOrigin:NSMakePoint(kPaddingHorizontal,
                                       (totalHeight - gripHeight) / 2)];

  // Create text label.
  int maximumWidth =
      std::min(kMaximumWidth, NSWidth([[NSScreen mainScreen] visibleFrame]));
  int maxLabelWidth = maximumWidth - kPaddingHorizontal * 2 -
                      kHorizontalMargin * 2 - gripWidth - buttonsWidth;
  gfx::FontList font_list;
  base::string16 elidedText =
      gfx::ElideText(text, font_list, maxLabelWidth, gfx::ELIDE_MIDDLE,
                     gfx::Typesetter::NATIVE);
  NSString* statusText = base::SysUTF16ToNSString(elidedText);
  base::scoped_nsobject<NSTextField> statusTextField(
      [[NSTextField alloc] initWithFrame:ui::kWindowSizeDeterminedLater]);
  [statusTextField setEditable:NO];
  [statusTextField setSelectable:NO];
  [statusTextField setDrawsBackground:NO];
  [statusTextField setBezeled:NO];
  [statusTextField setStringValue:statusText];
  [statusTextField setFont:font_list.GetPrimaryFont().GetNativeFont()];
  [statusTextField sizeToFit];
  [statusTextField setFrameOrigin:NSMakePoint(
                       kPaddingHorizontal + kHorizontalMargin + gripWidth,
                       (totalHeight - NSHeight([statusTextField frame])) / 2)];
  [content addSubview:statusTextField];

  // Resize content view to fit controls.
  CGFloat minimumLableWidth = kMinimumWidth - kPaddingHorizontal * 2 -
                              kHorizontalMargin * 2 - gripWidth - buttonsWidth;
  CGFloat lableWidth =
      std::max(NSWidth([statusTextField frame]), minimumLableWidth);
  CGFloat totalWidth = kPaddingHorizontal * 2 + kHorizontalMargin * 2 +
                       gripWidth + lableWidth + buttonsWidth;
  [content setFrame:NSMakeRect(0, 0, totalWidth, totalHeight)];

  // Move the buttons to the right place.
  NSPoint buttonOrigin = NSMakePoint(
      totalWidth - kPaddingHorizontal - buttonsWidth, kPaddingVertical);
  [stopButton_ setFrameOrigin:buttonOrigin];

  [minimizeButton_ setFrameOrigin:NSMakePoint(
      totalWidth - kPaddingHorizontal - NSWidth([minimizeButton_ frame]),
      (totalHeight - NSHeight([minimizeButton_ frame])) / 2)];

  if (base::i18n::IsRTL()) {
    [stopButton_
        setFrameOrigin:NSMakePoint(totalWidth - NSMaxX([stopButton_ frame]),
                                   NSMinY([stopButton_ frame]))];
    [minimizeButton_
        setFrameOrigin:NSMakePoint(totalWidth - NSMaxX([minimizeButton_ frame]),
                                   NSMinY([minimizeButton_ frame]))];
    [statusTextField
        setFrameOrigin:NSMakePoint(totalWidth - NSMaxX([statusTextField frame]),
                                   NSMinY([statusTextField frame]))];
    [gripView setFrameOrigin:NSMakePoint(totalWidth - NSMaxX([gripView frame]),
                                         NSMinY([gripView frame]))];
  }
}

- (void)windowWillClose:(NSNotification*)notification {
  [self stopSharing:nil];
}

@end

@implementation ScreenCaptureNotificationView

- (void)drawRect:(NSRect)dirtyRect {
  [skia::SkColorToSRGBNSColor(
      ui::NativeTheme::GetInstanceForNativeUi()->GetSystemColor(
          ui::NativeTheme::kColorId_DialogBackground)) set];
  [[NSBezierPath bezierPathWithRoundedRect:[self bounds]
                                   xRadius:kWindowCornerRadius
                                   yRadius:kWindowCornerRadius] fill];
}

@end

@implementation WindowGripView
- (WindowGripView*)init {
  gfx::Image gripImage =
      ui::ResourceBundle::GetSharedInstance().GetNativeImageNamed(
          IDR_SCREEN_CAPTURE_NOTIFICATION_GRIP);
  self = [super
      initWithFrame:NSMakeRect(0, 0, gripImage.Width(), gripImage.Height())];
  [self setImage:gripImage.ToNSImage()];
  return self;
}

- (BOOL)mouseDownCanMoveWindow {
  return YES;
}
@end
