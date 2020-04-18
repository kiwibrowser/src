// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/extension_installed_bubble_controller.h"

#include <stddef.h>

#include <memory>

#include "base/i18n/rtl.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/extensions/extension_action_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/bubble_anchor_util.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#include "chrome/browser/ui/cocoa/browser_window_cocoa.h"
#include "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/browser/ui/cocoa/bubble_anchor_helper_views.h"
#import "chrome/browser/ui/cocoa/bubble_sync_promo_controller.h"
#include "chrome/browser/ui/cocoa/chrome_style.h"
#include "chrome/browser/ui/cocoa/extensions/browser_actions_controller.h"
#include "chrome/browser/ui/cocoa/hover_close_button.h"
#include "chrome/browser/ui/cocoa/info_bubble_view.h"
#include "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#include "chrome/browser/ui/cocoa/new_tab_button.h"
#include "chrome/browser/ui/cocoa/tabs/tab_strip_view.h"
#include "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "chrome/browser/ui/extensions/extension_install_ui_factory.h"
#include "chrome/browser/ui/extensions/extension_installed_bubble.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/sync/sync_promo_ui.h"
#include "chrome/common/extensions/api/omnibox/omnibox_handler.h"
#include "chrome/common/extensions/sync_helper.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/bubble/bubble_controller.h"
#include "components/bubble/bubble_ui.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/install/extension_install_ui.h"
#include "extensions/common/extension.h"
#import "skia/ext/skia_utils_mac.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/base/cocoa/controls/hyperlink_text_view.h"
#include "ui/base/l10n/l10n_util.h"
#import "ui/gfx/mac/coordinate_conversion.h"

using content::BrowserThread;
using extensions::Extension;

@interface ExtensionInstalledBubbleController ()

- (const Extension*)extension;
- (void)windowWillClose:(NSNotification*)notification;
- (void)windowDidResignKey:(NSNotification*)notification;
- (NSPoint)calculateArrowPoint;
- (NSWindow*)initializeWindow;
- (int)calculateWindowHeight;
- (void)setMessageFrames:(int)newWindowHeight;
- (void)updateAnchorPosition;

@end  // ExtensionInstalledBubbleController ()

namespace {

class ExtensionInstalledBubbleBridge : public BubbleUi {
 public:
  explicit ExtensionInstalledBubbleBridge(
      ExtensionInstalledBubbleController* controller);
  ~ExtensionInstalledBubbleBridge() override;

 private:
  // BubbleUi:
  void Show(BubbleReference bubble_reference) override;
  void Close() override;
  void UpdateAnchorPosition() override;

  // Weak reference to the controller. |controller_| will outlive the bridge.
  ExtensionInstalledBubbleController* controller_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionInstalledBubbleBridge);
};

ExtensionInstalledBubbleBridge::ExtensionInstalledBubbleBridge(
    ExtensionInstalledBubbleController* controller)
    : controller_(controller) {
}

ExtensionInstalledBubbleBridge::~ExtensionInstalledBubbleBridge() {
}

void ExtensionInstalledBubbleBridge::Show(BubbleReference bubble_reference) {
  [controller_ setBubbleReference:bubble_reference];
  [controller_ showWindow:controller_];
}

void ExtensionInstalledBubbleBridge::Close() {
  [controller_ doClose];
}

void ExtensionInstalledBubbleBridge::UpdateAnchorPosition() {
  [controller_ updateAnchorPosition];
}

}  // namespace

// Cocoa specific implementation.
bool ExtensionInstalledBubble::ShouldShow() {
  return true;
}

gfx::Point ExtensionInstalledBubble::GetAnchorPoint(
    gfx::NativeWindow window) const {
  return bubble_anchor_util::GetExtensionInstalledAnchorPointCocoa(window,
                                                                   this);
}

// Implemented here to create the platform specific instance of the BubbleUi.
std::unique_ptr<BubbleUi> ExtensionInstalledBubble::BuildBubbleUi() {
  if (chrome::ShowAllDialogsWithViewsToolkit())
    return chrome::BuildViewsExtensionInstalledBubbleUi(this);

  // |controller| is owned by the parent window.
  ExtensionInstalledBubbleController* controller =
      [[ExtensionInstalledBubbleController alloc]
          initWithParentWindow:browser()->window()->GetNativeWindow()
               extensionBubble:this];

  // The bridge to the C++ object that performs shared logic across platforms.
  // This tells the controller when to show the bubble.
  return base::WrapUnique(new ExtensionInstalledBubbleBridge(controller));
}

@implementation ExtensionInstalledBubbleController

@synthesize installedBubble = installedBubble_;
// Exposed for unit tests.
@synthesize heading = heading_;
@synthesize closeButton = closeButton_;
@synthesize howToUse = howToUse_;
@synthesize howToManage = howToManage_;
@synthesize appInstalledShortcutLink = appInstalledShortcutLink_;
@synthesize manageShortcutLink = manageShortcutLink_;
@synthesize promoContainer = promoContainer_;
@synthesize iconImage = iconImage_;

- (id)initWithParentWindow:(NSWindow*)parentWindow
           extensionBubble:(ExtensionInstalledBubble*)extensionBubble {
  if ((self = [super initWithWindowNibPath:@"ExtensionInstalledBubble"
                              parentWindow:parentWindow
                                anchoredAt:NSZeroPoint])) {
    DCHECK(extensionBubble);
    const extensions::Extension* extension = extensionBubble->extension();
    browser_ = extensionBubble->browser();
    DCHECK(browser_);
    icon_.reset([skia::SkBitmapToNSImage(extensionBubble->icon()) retain]);

    type_ = extension->is_app() ? extension_installed_bubble::kApp :
        extension_installed_bubble::kExtension;

    installedBubble_ = extensionBubble;
  }
  return self;
}

- (const Extension*)extension {
  if (!installedBubble_)
    return nullptr;
  return installedBubble_->extension();
}

- (void)windowWillClose:(NSNotification*)notification {
  // Turn off page action icon preview when the window closes, unless we
  // already removed it when the window resigned key status.
  browser_ = nullptr;
  [closeButton_ setTrackingEnabled:NO];
  [super windowWillClose:notification];
}

// The controller is the delegate of the window, so it receives "did resign
// key" notifications.  When key is resigned, close the window.
- (void)windowDidResignKey:(NSNotification*)notification {
  // If the browser window is closing, we need to remove the page action
  // immediately, otherwise the closing animation may overlap with
  // browser destruction.
  [super windowDidResignKey:notification];
}

- (IBAction)closeWindow:(id)sender {
  DCHECK([[self window] isVisible]);
  DCHECK([self bubbleReference]);
  bool didClose =
      [self bubbleReference]->CloseBubble(BUBBLE_CLOSE_USER_DISMISSED);
  DCHECK(didClose);
}

// The extension installed bubble points at the browser action icon or the
// page action icon (shown as a preview), depending on the extension type.
// We need to calculate the location of these icons and the size of the
// message itself (which varies with the title of the extension) in order
// to figure out the origin point for the extension installed bubble.
- (NSPoint)calculateArrowPoint {
  BrowserWindowCocoa* window =
      static_cast<BrowserWindowCocoa*>(browser_->window());
  if (type_ == extension_installed_bubble::kApp) {
    TabStripView* view = [window->cocoa_controller() tabStripView];
    NewTabButton* button = [view getNewTabButton];
    NSRect bounds = [button bounds];
    NSPoint anchor = NSMakePoint(
        NSMidX(bounds),
        NSMaxY(bounds) - extension_installed_bubble::kAppsBubbleArrowOffset);
    return ui::ConvertPointFromWindowToScreen(
        window->GetNativeWindow(), [button convertPoint:anchor toView:nil]);
  }

  DCHECK(installedBubble_);
  return gfx::ScreenPointToNSPoint(
      installedBubble_->GetAnchorPoint(window->GetNativeWindow()));
}

// Override -[BaseBubbleController showWindow:] to tweak bubble location and
// set up UI elements.
- (void)showWindow:(id)sender {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Load nib and calculate height based on messages to be shown.
  NSWindow* window = [self initializeWindow];
  int newWindowHeight = [self calculateWindowHeight];
  [self.bubble setFrameSize:NSMakeSize(
      NSWidth([[window contentView] bounds]), newWindowHeight)];
  NSSize windowDelta = NSMakeSize(
      0, newWindowHeight - NSHeight([[window contentView] bounds]));
  windowDelta = [[window contentView] convertSize:windowDelta toView:nil];
  NSRect newFrame = [window frame];
  newFrame.size.height += windowDelta.height;
  [window setFrame:newFrame display:NO];

  // Now that we have resized the window, adjust y pos of the messages.
  [self setMessageFrames:newWindowHeight];

  // Find window origin, taking into account bubble size and arrow location.
  [self updateAnchorPosition];

  if (syncPromoController_) {
    signin_metrics::RecordSigninImpressionUserActionForAccessPoint(
        signin_metrics::AccessPoint::ACCESS_POINT_EXTENSION_INSTALL_BUBBLE);
  }
  [super showWindow:sender];
}

// Finish nib loading, set arrow location and load icon into window.  This
// function is exposed for unit testing.
- (NSWindow*)initializeWindow {
  NSWindow* window = [self window];  // completes nib load

  if (installedBubble_ &&
      installedBubble_->anchor_position() ==
          ExtensionInstalledBubble::ANCHOR_OMNIBOX) {
    [self.bubble setArrowLocation:info_bubble::kTopLeading];
  } else {
    [self.bubble setArrowLocation:info_bubble::kTopTrailing];
  }

  // Set appropriate icon, resizing if necessary.
  if ([icon_ size].width > extension_installed_bubble::kIconSize) {
    [icon_ setSize:NSMakeSize(extension_installed_bubble::kIconSize,
                              extension_installed_bubble::kIconSize)];
  }
  [iconImage_ setImage:icon_];
  [iconImage_ setNeedsDisplay:YES];
  return window;
}

// Calculate the height of each install message, resizing messages in their
// frames to fit window width.  Return the new window height, based on the
// total of all message heights.
- (int)calculateWindowHeight {
  // Adjust the window height to reflect the sum height of all messages
  // and vertical padding.
  // If there's few enough messages, the icon area may be larger than the
  // messages.
  int contentColumnHeight =
      2 * extension_installed_bubble::kOuterVerticalMargin;
  int iconColumnHeight = 2 * extension_installed_bubble::kOuterVerticalMargin +
                         NSHeight([iconImage_ frame]);

  CGFloat syncPromoHeight = 0;
  if (installedBubble_->options() & ExtensionInstalledBubble::SIGN_IN_PROMO) {
    signin_metrics::AccessPoint accessPoint =
       signin_metrics::AccessPoint::ACCESS_POINT_EXTENSION_INSTALL_BUBBLE;
    syncPromoController_.reset(
        [[BubbleSyncPromoController alloc]
            initWithBrowser:browser_
              promoStringId:IDS_EXTENSION_INSTALLED_SYNC_PROMO_NEW
               linkStringId:IDS_EXTENSION_INSTALLED_SYNC_PROMO_LINK_NEW
                accessPoint:accessPoint]);
    [promoContainer_ addSubview:[syncPromoController_ view]];

    // Resize the sync promo and its placeholder.
    NSRect syncPromoPlaceholderFrame = [promoContainer_ frame];
    CGFloat windowWidth = NSWidth([[self bubble] frame]);
    syncPromoPlaceholderFrame.size.width = windowWidth;
    syncPromoHeight =
        [syncPromoController_ preferredHeightForWidth:windowWidth];
    syncPromoPlaceholderFrame.size.height = syncPromoHeight;

    [promoContainer_ setFrame:syncPromoPlaceholderFrame];
    [[syncPromoController_ view] setFrame:syncPromoPlaceholderFrame];
  } else {
    [promoContainer_ setHidden:YES];
  }

  // First part of extension installed message, the heading.
  base::string16 extension_name =
      base::UTF8ToUTF16([self extension]->name().c_str());
  base::i18n::AdjustStringForLocaleDirection(&extension_name);
  [heading_ setStringValue:l10n_util::GetNSStringF(
      IDS_EXTENSION_INSTALLED_HEADING, extension_name)];
  [GTMUILocalizerAndLayoutTweaker
      sizeToFitFixedWidthTextField:heading_];
  contentColumnHeight += NSHeight([heading_ frame]);

  if (installedBubble_->options() & ExtensionInstalledBubble::HOW_TO_USE) {
    [howToUse_ setStringValue:base::SysUTF16ToNSString(
         installedBubble_->GetHowToUseDescription())];
    [howToUse_ setHidden:NO];
    [[howToUse_ cell]
        setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
    [GTMUILocalizerAndLayoutTweaker
        sizeToFitFixedWidthTextField:howToUse_];
    contentColumnHeight += NSHeight([howToUse_ frame]) +
        extension_installed_bubble::kInnerVerticalMargin;
  }

  // If type is app, hide howToManage_, and include a "show me" link in the
  // bubble.
  if (type_ == extension_installed_bubble::kApp) {
    [howToManage_ setHidden:YES];
    [appShortcutLink_ setHidden:NO];
    contentColumnHeight += 2 * extension_installed_bubble::kInnerVerticalMargin;
    contentColumnHeight += NSHeight([appShortcutLink_ frame]);
  } else if (installedBubble_->options() &
                 ExtensionInstalledBubble::HOW_TO_MANAGE) {
    // Second part of extension installed message.
    [[howToManage_ cell]
        setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
    [GTMUILocalizerAndLayoutTweaker
        sizeToFitFixedWidthTextField:howToManage_];
    contentColumnHeight += NSHeight([howToManage_ frame]) +
        extension_installed_bubble::kInnerVerticalMargin;
  } else {
    [howToManage_ setHidden:YES];
  }

  // Sync sign-in promo, if any.
  if (syncPromoHeight > 0) {
    // The sync promo goes at the bottom of the window and includes its own
    // bottom margin. Thus, we subtract off the one of the outer margins, and
    // apply it to both the icon area and content area.
    int syncPromoDelta = extension_installed_bubble::kInnerVerticalMargin +
                         syncPromoHeight -
                         extension_installed_bubble::kOuterVerticalMargin;
    contentColumnHeight += syncPromoDelta;
    iconColumnHeight += syncPromoDelta;
  }

  if (installedBubble_->options() & ExtensionInstalledBubble::SHOW_KEYBINDING) {
    [manageShortcutLink_ setHidden:NO];
    [[manageShortcutLink_ cell]
        setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
    [[manageShortcutLink_ cell]
        setTextColor:skia::SkColorToCalibratedNSColor(
            chrome_style::GetLinkColor())];
    [GTMUILocalizerAndLayoutTweaker sizeToFitView:manageShortcutLink_];
    contentColumnHeight += extension_installed_bubble::kInnerVerticalMargin;
    contentColumnHeight += NSHeight([manageShortcutLink_ frame]);
  }

  return std::max(contentColumnHeight, iconColumnHeight);
}

// Adjust y-position of messages to sit properly in new window height.
- (void)setMessageFrames:(int)newWindowHeight {
  NSRect headingFrame = [heading_ frame];
  headingFrame.origin.y = newWindowHeight - (
      NSHeight(headingFrame) +
      extension_installed_bubble::kOuterVerticalMargin);
  [heading_ setFrame:headingFrame];
  int nextY = NSMinY(headingFrame);

  auto adjustView = [](NSView* view, int* nextY) {
    DCHECK(nextY);
    NSRect frame = [view frame];
    frame.origin.y = *nextY -
        (NSHeight(frame) + extension_installed_bubble::kInnerVerticalMargin);
    [view setFrame:frame];
    *nextY = NSMinY(frame);
  };

  if (installedBubble_->options() & ExtensionInstalledBubble::HOW_TO_USE)
    adjustView(howToUse_, &nextY);

  if (installedBubble_->options() & ExtensionInstalledBubble::HOW_TO_MANAGE)
    adjustView(howToManage_, &nextY);

  if (installedBubble_->options() & ExtensionInstalledBubble::SHOW_KEYBINDING)
    adjustView(manageShortcutLink_, &nextY);

  if (installedBubble_->options() & ExtensionInstalledBubble::SIGN_IN_PROMO) {
    // The sync promo goes at the bottom of the bubble, but that might be
    // different than directly below the previous content if the icon is larger
    // than the messages. Workaround by just always setting nextY to be at the
    // bottom.
    nextY = NSHeight([promoContainer_ frame]) +
            extension_installed_bubble::kInnerVerticalMargin;
    adjustView(promoContainer_, &nextY);
  }
}

- (void)updateAnchorPosition {
  self.anchorPoint = [self calculateArrowPoint];
}

- (IBAction)onManageShortcutClicked:(id)sender {
  DCHECK([self bubbleReference]);
  bool didClose = [self bubbleReference]->CloseBubble(BUBBLE_CLOSE_ACCEPTED);
  DCHECK(didClose);
  std::string configure_url = chrome::kChromeUIExtensionsURL;
  configure_url += chrome::kExtensionConfigureCommandsSubPage;
  NavigateParams params(
      GetSingletonTabNavigateParams(browser_, GURL(configure_url)));
  Navigate(&params);
}

- (IBAction)onAppShortcutClicked:(id)sender {
  std::unique_ptr<extensions::ExtensionInstallUI> install_ui(
      extensions::CreateExtensionInstallUI(browser_->profile()));
  install_ui->OpenAppInstalledUI([self extension]->id());
}

- (void)doClose {
  installedBubble_ = nullptr;
  [self close];
}

@end
