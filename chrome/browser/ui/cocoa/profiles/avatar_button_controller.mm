// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/profiles/avatar_button_controller.h"

#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/sync/sync_ui_util.h"
#import "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/profiles/avatar_button.h"
#include "chrome/grit/generated_resources.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "skia/ext/skia_utils_mac.h"
#import "ui/base/cocoa/appkit_utils.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

namespace {

constexpr SkColor kButtonHoverColor = SkColorSetARGB(20, 0, 0, 0);
constexpr SkColor kButtonPressedColor = SkColorSetARGB(31, 0, 0, 0);

const CGFloat kButtonHeight = 24;

// NSButtons have a default padding of 5px. Buttons should have a padding of
// 6px.
const CGFloat kButtonExtraPadding = 6 - 5;

// Extra padding for the signed out avatar button.
const CGFloat kSignedOutWidthPadding = 2;

// Kern value for the avatar button title.
const CGFloat kTitleKern = 0.25;

// Upper and lower bounds for determining if the frame's theme color is a
// "dark" theme. This value is determined by trial and error.
const CGFloat kFrameColorDarkUpperBound = 0.33;

}  // namespace

// Button cell with a custom border given by a set of nine-patch image grids.
@interface CustomThemeButtonCell : NSButtonCell {
 @private
   BOOL isThemedWindow_;
   BOOL hasError_;
}
- (void)setIsThemedWindow:(BOOL)isThemedWindow;
- (void)setHasError:(BOOL)hasError withTitle:(NSString*)title;

@end

@implementation CustomThemeButtonCell
- (id)initWithThemedWindow:(BOOL)isThemedWindow {
  if ((self = [super init])) {
    isThemedWindow_ = isThemedWindow;
    hasError_ = false;
  }
  return self;
}

- (NSSize)cellSize {
  NSSize buttonSize = [super cellSize];

  // An image and no error means we are drawing the generic button, which
  // is square. Otherwise, we are displaying the profile's name and an
  // optional authentication error icon.
  if ([self image] && !hasError_)
    buttonSize.width = kButtonHeight + kSignedOutWidthPadding;
  else
    buttonSize.width += 2 * kButtonExtraPadding;
  buttonSize.height = kButtonHeight;
  return buttonSize;
}

- (void)drawInteriorWithFrame:(NSRect)frame inView:(NSView*)controlView {
  NSRect frameAfterPadding = NSInsetRect(frame, kButtonExtraPadding, 0);
  [super drawInteriorWithFrame:frameAfterPadding inView:controlView];
}

- (void)drawImage:(NSImage*)image
        withFrame:(NSRect)frame
           inView:(NSView*)controlView {
  // The image used in the generic button case as well as the error icon both
  // need to be shifted down slightly to be centered correctly.
  // TODO(noms): When the assets are fixed, remove this latter offset.
  frame = NSOffsetRect(frame, 0, 1);
  [super drawImage:image withFrame:frame inView:controlView];
}

- (void)drawBezelWithFrame:(NSRect)frame
                    inView:(NSView*)controlView {
  AvatarButton* button = base::mac::ObjCCastStrict<AvatarButton>(controlView);
  HoverState hoverState = [button hoverState];

  NSColor* backgroundColor = nil;
  if (hoverState == kHoverStateMouseDown || [button isActive]) {
    backgroundColor = skia::SkColorToSRGBNSColor(kButtonPressedColor);
  } else if (hoverState == kHoverStateMouseOver) {
    backgroundColor = skia::SkColorToSRGBNSColor(kButtonHoverColor);
  }

  if (backgroundColor) {
    [backgroundColor set];
    NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:frame
                                                         xRadius:2.0f
                                                         yRadius:2.0f];
    [path fill];
  }
}

- (void)drawFocusRingMaskWithFrame:(NSRect)frame inView:(NSView*)view {
  // Match the bezel's shape.
  [[NSBezierPath bezierPathWithRoundedRect:NSInsetRect(frame, 2, 2)
                                   xRadius:2
                                   yRadius:2] fill];
}

- (void)setIsThemedWindow:(BOOL)isThemedWindow {
  isThemedWindow_ = isThemedWindow;
}

- (void)setHasError:(BOOL)hasError withTitle:(NSString*)title {
  hasError_ = hasError;
  int messageId = hasError ?
      IDS_PROFILES_ACCOUNT_BUTTON_AUTH_ERROR_ACCESSIBLE_NAME :
      IDS_PROFILES_NEW_AVATAR_BUTTON_ACCESSIBLE_NAME;

  [self accessibilitySetOverrideValue:l10n_util::GetNSStringF(
      messageId, base::SysNSStringToUTF16(title))
                         forAttribute:NSAccessibilityTitleAttribute];
}

@end

@interface AvatarButtonController (Private)
- (void)updateAvatarButtonAndLayoutParent:(BOOL)layoutParent;
- (void)setErrorStatus:(BOOL)hasError;
- (void)dealloc;
- (void)themeDidChangeNotification:(NSNotification*)aNotification;

// Called right after |window_| became/resigned the main window.
- (void)mainWindowDidChangeNotification:(NSNotification*)aNotification;
@end

@implementation AvatarButtonController

- (id)initWithBrowser:(Browser*)browser window:(NSWindow*)window {
  if ((self = [super initWithBrowser:browser])) {
    ThemeService* themeService =
        ThemeServiceFactory::GetForProfile(browser->profile());
    isThemedWindow_ = !themeService->UsingSystemTheme();

    AvatarButton* avatarButton =
        [[AvatarButton alloc] initWithFrame:NSZeroRect];
    avatarButton.sendActionOnMouseDown = YES;
    button_.reset(avatarButton);

    base::scoped_nsobject<NSButtonCell> cell(
        [[CustomThemeButtonCell alloc] initWithThemedWindow:isThemedWindow_]);

    [avatarButton setCell:cell.get()];

    [avatarButton setWantsLayer:YES];
    [self setView:avatarButton];

    [avatarButton setBezelStyle:NSShadowlessSquareBezelStyle];
    [avatarButton setButtonType:NSMomentaryChangeButton];
    [[avatarButton cell] setHighlightsBy:NSNoCellMask];
    [avatarButton setBordered:YES];

    [avatarButton setTarget:self];
    [avatarButton setAction:@selector(buttonClicked:)];
    [avatarButton setRightAction:@selector(buttonClicked:)];

    // Check if the account already has an authentication or sync error and
    // initialize the avatar button UI.
    hasError_ = profileObserver_->HasAvatarError();
    [self updateAvatarButtonAndLayoutParent:NO];

    window_ = window;

    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self
               selector:@selector(themeDidChangeNotification:)
                   name:kBrowserThemeDidChangeNotification
                 object:nil];

    [center addObserver:self
               selector:@selector(mainWindowDidChangeNotification:)
                   name:NSWindowDidBecomeMainNotification
                 object:window];
    [center addObserver:self
               selector:@selector(mainWindowDidChangeNotification:)
                   name:NSWindowDidResignMainNotification
                 object:window];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)themeDidChangeNotification:(NSNotification*)aNotification {
  ThemeService* themeService =
      ThemeServiceFactory::GetForProfile(browser_->profile());
  BOOL updatedIsThemedWindow = !themeService->UsingSystemTheme();
  isThemedWindow_ = updatedIsThemedWindow;
  [[button_ cell] setIsThemedWindow:isThemedWindow_];
  [self updateAvatarButtonAndLayoutParent:YES];
}

- (void)mainWindowDidChangeNotification:(NSNotification*)aNotification {
  [self updateAvatarButtonAndLayoutParent:NO];
}

- (void)updateAvatarButtonAndLayoutParent:(BOOL)layoutParent {
  // The button text has a black foreground and a white drop shadow for regular
  // windows, and a light text with a dark drop shadow for guest windows
  // which are themed with a dark background.
  NSColor* foregroundColor =
      [self isFrameColorDark] ? [NSColor whiteColor] : [NSColor blackColor];

  bool useGenericButton = [self shouldUseGenericButton];

  NSString* buttonTitle = base::SysUTF16ToNSString(useGenericButton ?
      base::string16() :
      profiles::GetAvatarButtonTextForProfile(browser_->profile()));
  [[button_ cell] setHasError:hasError_ withTitle:buttonTitle];

  AvatarButton* button =
      base::mac::ObjCCastStrict<AvatarButton>(button_);

  if (useGenericButton) {
    NSImage* avatarIcon = NSImageFromImageSkia(gfx::CreateVectorIcon(
        kUserAccountAvatarIcon, 18, gfx::kChromeIconGrey));
    [button setDefaultImage:avatarIcon];
    [button setHoverImage:nil];
    [button setPressedImage:nil];
    [button setImagePosition:NSImageOnly];
  } else if (hasError_) {
    // When DICE is enabled and the error is an auth error, the sync-paused icon
    // is shown.
    int dummy;
    const bool should_show_sync_paused_ui =
        AccountConsistencyModeManager::IsDiceEnabledForProfile(
            browser_->profile()) &&
        sync_ui_util::GetMessagesForAvatarSyncError(
            browser_->profile(),
            *SigninManagerFactory::GetForProfile(browser_->profile()), &dummy,
            &dummy) == sync_ui_util::AUTH_ERROR;
    NSImage* errorIcon = NSImageFromImageSkia(
        should_show_sync_paused_ui
            ? gfx::CreateVectorIcon(kSyncPausedIcon, 16, gfx::kGoogleBlue500)
            : gfx::CreateVectorIcon(kSyncProblemIcon, 16, gfx::kGoogleRed700));
    [button setDefaultImage:errorIcon];
    [button setHoverImage:nil];
    [button setPressedImage:nil];
    [button setImage:errorIcon];
    [button setImagePosition:NSImageLeft];
  } else {
    [button setDefaultImage:nil];
    [button setHoverImage:nil];
    [button setPressedImage:nil];
    [button setImagePosition:NSNoImage];
  }

  base::scoped_nsobject<NSMutableParagraphStyle> paragraphStyle(
      [[NSMutableParagraphStyle alloc] init]);
  [paragraphStyle setAlignment:NSLeftTextAlignment];

  base::scoped_nsobject<NSAttributedString> attributedTitle(
      [[NSAttributedString alloc]
          initWithString:buttonTitle
              attributes:@{
                NSForegroundColorAttributeName : foregroundColor,
                NSParagraphStyleAttributeName : paragraphStyle,
                NSKernAttributeName : @(kTitleKern),
              }]);
  [button_ setAttributedTitle:attributedTitle];
  [button_ sizeToFit];

  if (layoutParent) {
    // Because the width of the button might have changed, the parent browser
    // frame needs to recalculate the button bounds and redraw it.
    [[BrowserWindowController
        browserWindowControllerForWindow:browser_->window()->GetNativeWindow()]
        layoutSubviews];
  }
}

- (void)setErrorStatus:(BOOL)hasError {
  hasError_ = hasError;
  [self updateAvatarButtonAndLayoutParent:YES];
}

- (BOOL)isFrameColorDark {
  const ui::ThemeProvider* themeProvider =
      &ThemeService::GetThemeProviderForProfile(browser_->profile());
  const int propertyId = [window_ isMainWindow]
                             ? ThemeProperties::COLOR_FRAME
                             : ThemeProperties::COLOR_FRAME_INACTIVE;
  if (themeProvider && themeProvider->HasCustomColor(propertyId)) {
    NSColor* frameColor = themeProvider->GetNSColor(propertyId);
    frameColor =
        [frameColor colorUsingColorSpaceName:NSCalibratedWhiteColorSpace];
    return frameColor &&
           [frameColor whiteComponent] < kFrameColorDarkUpperBound;
  }

  return false;
}

- (void)showAvatarBubbleAnchoredAt:(NSView*)anchor
                          withMode:(BrowserWindow::AvatarBubbleMode)mode
                   withServiceType:(signin::GAIAServiceType)serviceType
                   fromAccessPoint:(signin_metrics::AccessPoint)accessPoint {
  [super showAvatarBubbleAnchoredAt:anchor
                           withMode:mode
                    withServiceType:serviceType
                    fromAccessPoint:accessPoint];

  AvatarButton* button = base::mac::ObjCCastStrict<AvatarButton>(button_);
  // When the user clicks a second time on the button, the menu closes.
  [button setIsActive:[self isMenuOpened]];
}

// AvatarBaseController overrides:
- (void)bubbleWillClose {
  AvatarButton* button = base::mac::ObjCCastStrict<AvatarButton>(button_);
  [button setIsActive:NO];
  [self updateAvatarButtonAndLayoutParent:NO];
  [super bubbleWillClose];
}

@end
