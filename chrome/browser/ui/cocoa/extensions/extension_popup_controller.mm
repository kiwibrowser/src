// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/extension_popup_controller.h"

#include <algorithm>
#include <utility>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/extensions/extension_view_host.h"
#include "chrome/browser/extensions/extension_view_host_factory.h"
#include "chrome/browser/ui/browser.h"
#import "chrome/browser/ui/cocoa/browser_window_cocoa.h"
#import "chrome/browser/ui/cocoa/extensions/extension_view_mac.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#import "chrome/browser/ui/cocoa/l10n_util.h"
#include "chrome/common/url_constants.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_agent_host_observer.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "extensions/browser/notification_types.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/base/cocoa/window_size_constants.h"
#include "ui/base/material_design/material_design_controller.h"

using content::BrowserContext;
using content::RenderViewHost;
using content::WebContents;
using extensions::ExtensionViewHost;

namespace {

// The duration for any animations that might be invoked by this controller.
const NSTimeInterval kAnimationDuration = 0.2;

// There should only be one extension popup showing at one time. Keep a
// reference to it here.
ExtensionPopupController* gPopup;

// Given a value and a rage, clamp the value into the range.
CGFloat Clamp(CGFloat value, CGFloat min, CGFloat max) {
  return std::max(min, std::min(max, value));
}

BOOL gAnimationsEnabled = true;

}  // namespace

@interface ExtensionPopupController (Private)
// Callers should be using the public static method for initialization.
- (id)initWithParentWindow:(NSWindow*)parentWindow
                anchoredAt:(NSPoint)anchoredAt
                   devMode:(BOOL)devMode;

// Set the ExtensionViewHost, taking ownership.
- (void)setExtensionViewHost:(std::unique_ptr<ExtensionViewHost>)host;

// Called when the extension's hosted NSView has been resized.
- (void)extensionViewFrameChanged;

// Called when the extension's size changes.
- (void)onSizeChanged:(NSSize)newSize;

// Called when the extension view is shown.
- (void)onViewDidShow;

@end

class ExtensionPopupContainer : public ExtensionViewMac::Container {
 public:
  explicit ExtensionPopupContainer(ExtensionPopupController* controller)
      : controller_(controller) {
  }

  void OnExtensionSizeChanged(ExtensionViewMac* view,
                              const gfx::Size& new_size) override {
    [controller_ onSizeChanged:
        NSMakeSize(new_size.width(), new_size.height())];
  }

  void OnExtensionViewDidShow(ExtensionViewMac* view) override {
    [controller_ onViewDidShow];
  }

 private:
  ExtensionPopupController* controller_; // Weak; owns this.
};

class ExtensionPopupNotificationBridge :
    public content::NotificationObserver,
    public content::DevToolsAgentHostObserver {
 public:
  ExtensionPopupNotificationBridge(ExtensionPopupController* controller,
                                   ExtensionViewHost* view_host)
    : controller_(controller),
      view_host_(view_host),
      web_contents_(view_host_->host_contents()) {
    content::DevToolsAgentHost::AddObserver(this);
  }

  ~ExtensionPopupNotificationBridge() override {
    content::DevToolsAgentHost::RemoveObserver(this);
  }

  void DevToolsAgentHostAttached(
      content::DevToolsAgentHost* agent_host) override {
    if (agent_host->GetWebContents() != web_contents_)
      return;
    // Set the flag on the controller so the popup is not hidden when
    // the dev tools get focus.
    [controller_ setBeingInspected:YES];
  }

  void DevToolsAgentHostDetached(
      content::DevToolsAgentHost* agent_host) override {
    if (agent_host->GetWebContents() != web_contents_)
      return;
    // Allow the devtools to finish detaching before we close the popup.
    [controller_ performSelector:@selector(close)
                      withObject:nil
                      afterDelay:0.0];
  }

  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    switch (type) {
      case extensions::NOTIFICATION_EXTENSION_HOST_DID_STOP_FIRST_LOAD:
        if (content::Details<ExtensionViewHost>(view_host_) == details)
          [controller_ showDevTools];
        break;
      case extensions::NOTIFICATION_EXTENSION_HOST_VIEW_SHOULD_CLOSE:
        if (content::Details<ExtensionViewHost>(view_host_) == details &&
            ![controller_ isClosing]) {
          [controller_ close];
        }
        break;
      default:
        NOTREACHED() << "Received unexpected notification";
        break;
    }
  }

 private:
  ExtensionPopupController* controller_;

  extensions::ExtensionViewHost* view_host_;

  // WebContents for controller. Hold onto this separately because we need to
  // know what it is for notifications, but our ExtensionViewHost may not be
  // valid.
  WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionPopupNotificationBridge);
};

@implementation ExtensionPopupController

@synthesize extensionId = extensionId_;

- (id)initWithParentWindow:(NSWindow*)parentWindow
                anchoredAt:(NSPoint)anchoredAt
                   devMode:(BOOL)devMode {
  base::scoped_nsobject<InfoBubbleWindow> window([[InfoBubbleWindow alloc]
      initWithContentRect:ui::kWindowSizeDeterminedLater
                styleMask:NSBorderlessWindowMask
                  backing:NSBackingStoreBuffered
                    defer:NO]);
  if (!window.get())
    return nil;

  anchoredAt = ui::ConvertPointFromWindowToScreen(parentWindow, anchoredAt);
  if ((self = [super initWithWindow:window
                       parentWindow:parentWindow
                         anchoredAt:anchoredAt])) {
    beingInspected_ = devMode;
    ignoreWindowDidResignKey_ = NO;
    if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
      // Under MD, bubbles never have arrows.
      [[self bubble] setArrowLocation:info_bubble::kNoArrow];
      [[self bubble] setAlignment:info_bubble::kAlignTrailingEdgeToAnchorEdge];
    } else {
      [[self bubble] setArrowLocation:info_bubble::kTopTrailing];
      [[self bubble] setAlignment:info_bubble::kAlignArrowToAnchor];
    }
    if (!gAnimationsEnabled)
      [window setAllowedAnimations:info_bubble::kAnimateNone];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)showDevTools {
  DevToolsWindow::OpenDevToolsWindow(host_->host_contents());
}

- (void)close {
  // |windowWillClose:| could have already been called. http://crbug.com/279505
  if (host_) {
    // TODO(gbillock): Change this API to say directly if the current popup
    // should block tab close? This is a bit over-reaching.
    const web_modal::WebContentsModalDialogManager* manager =
        web_modal::WebContentsModalDialogManager::FromWebContents(
            host_->host_contents());
    if (manager && manager->IsDialogActive())
      return;
  }
  [super close];
}

- (void)windowWillClose:(NSNotification *)notification {
  [super windowWillClose:notification];
  if (gPopup == self)
    gPopup = nil;
  if (host_->view())
    static_cast<ExtensionViewMac*>(host_->view())->set_container(NULL);
  host_.reset();
}

- (void)windowDidResignKey:(NSNotification*)notification {
  // |windowWillClose:| could have already been called. http://crbug.com/279505
  if (host_) {
    // When a modal dialog is opened on top of the popup and when it's closed,
    // it steals key-ness from the popup. Don't close the popup when this
    // happens. There's an extra windowDidResignKey: notification after the
    // modal dialog closes that should also be ignored.
    const web_modal::WebContentsModalDialogManager* manager =
        web_modal::WebContentsModalDialogManager::FromWebContents(
            host_->host_contents());
    if (manager && manager->IsDialogActive()) {
      ignoreWindowDidResignKey_ = YES;
      return;
    }
    if (ignoreWindowDidResignKey_) {
      ignoreWindowDidResignKey_ = NO;
      return;
    }
  }
  if (!beingInspected_)
    [super windowDidResignKey:notification];
}

- (BOOL)isClosing {
  return [static_cast<InfoBubbleWindow*>([self window]) isClosing];
}

- (ExtensionViewHost*)extensionViewHost {
  return host_.get();
}

- (void)setBeingInspected:(BOOL)beingInspected {
  beingInspected_ = beingInspected;
}

+ (ExtensionPopupController*)host:(std::unique_ptr<ExtensionViewHost>)host
                        inBrowser:(Browser*)browser
                       anchoredAt:(NSPoint)anchoredAt
                          devMode:(BOOL)devMode {
  DCHECK([NSThread isMainThread]);
  DCHECK(browser);
  DCHECK(host);

  if (gPopup)
    [gPopup close];  // Starts the animation to fade out the popup.

  // Create the popup first. This establishes an initially hidden NSWindow so
  // that the renderer is able to gather correct screen metrics for the initial
  // paint.
  gPopup = [[ExtensionPopupController alloc]
      initWithParentWindow:browser->window()->GetNativeWindow()
                anchoredAt:anchoredAt
                   devMode:devMode];
  [gPopup setExtensionViewHost:std::move(host)];
  return gPopup;
}

+ (ExtensionPopupController*)popup {
  return gPopup;
}

- (void)setExtensionViewHost:(std::unique_ptr<ExtensionViewHost>)host {
  DCHECK(!host_);
  DCHECK(host);
  host_.swap(host);

  extensionId_ = host_->extension_id();
  container_.reset(new ExtensionPopupContainer(self));
  ExtensionViewMac* hostView = static_cast<ExtensionViewMac*>(host_->view());
  hostView->set_container(container_.get());
  hostView->CreateWidgetHostViewIn([self bubble]);

  extensionView_ = hostView->GetNativeView();

  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  [center addObserver:self
             selector:@selector(extensionViewFrameChanged)
                 name:NSViewFrameDidChangeNotification
               object:extensionView_];

  notificationBridge_.reset(
      new ExtensionPopupNotificationBridge(self, host_.get()));
  content::Source<BrowserContext> source_context(host_->browser_context());
  registrar_.Add(notificationBridge_.get(),
                extensions::NOTIFICATION_EXTENSION_HOST_VIEW_SHOULD_CLOSE,
                source_context);
  if (beingInspected_) {
    // Listen for the extension to finish loading so the dev tools can be
    // opened.
    registrar_.Add(notificationBridge_.get(),
                   extensions::NOTIFICATION_EXTENSION_HOST_DID_STOP_FIRST_LOAD,
                   source_context);
  }
}

- (void)extensionViewFrameChanged {
  // If there are no changes in the width or height of the frame, then ignore.
  if (NSEqualSizes([extensionView_ frame].size, extensionFrame_.size))
    return;

  extensionFrame_ = [extensionView_ frame];
  // Constrain the size of the view.
  [extensionView_ setFrameSize:NSMakeSize(
      Clamp(NSWidth(extensionFrame_),
            ExtensionViewMac::kMinWidth,
            ExtensionViewMac::kMaxWidth),
      Clamp(NSHeight(extensionFrame_),
            ExtensionViewMac::kMinHeight,
            ExtensionViewMac::kMaxHeight))];

  // Pad the window by half of the rounded corner radius to prevent the
  // extension's view from bleeding out over the corners.
  CGFloat inset = info_bubble::kBubbleCornerRadius / 2.0;
  [extensionView_ setFrameOrigin:NSMakePoint(inset, inset)];

  NSRect frame = [extensionView_ frame];
  frame.size.height += info_bubble::kBubbleCornerRadius;
  frame.size.width += info_bubble::kBubbleCornerRadius;

  // Adjust the origin according to the height and width so that the arrow is
  // positioned correctly at the middle and slightly down from the button.
  NSPoint windowOrigin = self.anchorPoint;
  NSSize offsets = {0, 0};
  if ([[self bubble] arrowLocation] != info_bubble::kNoArrow) {
    frame.size.height += info_bubble::kBubbleArrowHeight;
    offsets = NSMakeSize(
        info_bubble::kBubbleArrowXOffset + info_bubble::kBubbleArrowWidth / 2.0,
        info_bubble::kBubbleArrowHeight / 2.0);
  }

  frame = [extensionView_ convertRect:frame toView:nil];
  offsets = [extensionView_ convertSize:offsets toView:nil];
  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
    windowOrigin.x -= offsets.width;
  } else {
    windowOrigin.x -= NSWidth(frame) - offsets.width;
  }
  windowOrigin.y -= NSHeight(frame) - offsets.height;
  frame.origin = windowOrigin;

  // Is the window still animating in or out? If so, then cancel that and create
  // a new animation setting the opacity and new frame value. Otherwise the
  // current animation will continue after this frame is set, reverting the
  // frame to what it was when the animation started.
  NSWindow* window = [self window];
  CGFloat targetAlpha = [self isClosing] ? 0.0 : 1.0;
  id animator = [window animator];
  if ([window isVisible] &&
      ([animator alphaValue] != targetAlpha ||
       !NSEqualRects([window frame], [animator frame]))) {
    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext] setDuration:kAnimationDuration];
    [animator setAlphaValue:targetAlpha];
    [animator setFrame:frame display:YES];
    [NSAnimationContext endGrouping];
  } else {
    [window setFrame:frame display:YES];
  }

  // A NSViewFrameDidChangeNotification won't be sent until the extension view
  // content is loaded. The window is hidden on init, so show it the first time
  // the notification is fired (and consequently the view contents have loaded).
  if (![window isVisible]) {
    [self showWindow:self];
  }
}

- (void)onSizeChanged:(NSSize)newSize {
  // When we update the size, the window will become visible. Stay hidden until
  // the host is loaded.
  pendingSize_ = newSize;
  if (!host_ || !host_->has_loaded_once())
    return;

  // No need to use CA here, our caller calls us repeatedly to animate the
  // resizing.
  NSRect frame = [extensionView_ frame];
  frame.size = newSize;

  // |new_size| is in pixels. Convert to view units.
  frame.size = [extensionView_ convertSize:frame.size fromView:nil];

  [extensionView_ setFrame:frame];
  [extensionView_ setNeedsDisplay:YES];
}

- (void)onViewDidShow {
  [self onSizeChanged:pendingSize_];
}

// Private (TestingAPI)
+ (void)setAnimationsEnabledForTesting:(BOOL)enabled {
  gAnimationsEnabled = enabled;
}

// Private (TestingAPI)
- (NSView*)view {
  return extensionView_;
}

// Private (TestingAPI)
+ (NSSize)minPopupSize {
  NSSize minSize = {ExtensionViewMac::kMinWidth, ExtensionViewMac::kMinHeight};
  return minSize;
}

// Private (TestingAPI)
+ (NSSize)maxPopupSize {
  NSSize maxSize = {ExtensionViewMac::kMaxWidth, ExtensionViewMac::kMaxHeight};
  return maxSize;
}

@end
