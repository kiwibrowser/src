// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/browser_action_button.h"

#include <algorithm>
#include <cmath>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/app_menu/app_menu_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/extensions/browser_actions_controller.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "chrome/browser/ui/toolbar/toolbar_action_button_state.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_delegate.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar.h"
#include "skia/ext/skia_utils_mac.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSAnimation+Duration.h"
#import "ui/base/cocoa/menu_controller.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/canvas_skia_paint.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"

NSString* const kBrowserActionButtonDraggingNotification =
    @"BrowserActionButtonDraggingNotification";
NSString* const kBrowserActionButtonDragEndNotification =
    @"BrowserActionButtonDragEndNotification";

static const CGFloat kAnimationDuration = 0.2;
static const CGFloat kMinimumDragDistance = 5;

// Mirrors ui/views/mouse_constants.h for suppressing popup activation.
static const int kMinimumMsBetweenCloseOpenPopup = 100;

@interface BrowserActionButton ()
- (void)endDrag;
- (void)updateHighlightedState;
- (MenuControllerCocoa*)contextMenuController;
- (void)menuDidClose:(NSNotification*)notification;
@end

// A class to bridge the ToolbarActionViewController and the
// BrowserActionButton.
class ToolbarActionViewDelegateBridge : public ToolbarActionViewDelegate {
 public:
  ToolbarActionViewDelegateBridge(BrowserActionButton* owner,
                                  BrowserActionsController* controller,
                                  ToolbarActionViewController* viewController);
  ~ToolbarActionViewDelegateBridge() override;

  // Shows the context menu for the owning action.
  void ShowContextMenu();

  bool user_shown_popup_visible() const { return user_shown_popup_visible_; }

  bool CanShowPopup() const {
    // If the user clicks on the browser action button to close the bubble,
    // don't show the next popup too soon on the mouse button up.
    base::TimeDelta delta = base::TimeTicks::Now() - popup_closed_time_;
    return delta.InMilliseconds() >= kMinimumMsBetweenCloseOpenPopup;
  }

 private:
  // ToolbarActionViewDelegate:
  content::WebContents* GetCurrentWebContents() const override;
  void UpdateState() override;
  bool IsMenuRunning() const override;
  void OnPopupShown(bool by_user) override;
  void OnPopupClosed() override;

  // A helper method to implement showing the context menu.
  void DoShowContextMenu();

  // Tracks when the menu was closed so that the button can ignore the incoming
  // mouse button up event if its too soon. This simulates the same behavior
  // provided by the views toolkit MenuButton Pressed Locked tracking state.
  base::TimeTicks popup_closed_time_;

  // The owning button. Weak.
  BrowserActionButton* owner_;

  // The BrowserActionsController that owns the button. Weak.
  BrowserActionsController* controller_;

  // The ToolbarActionViewController for which this is the delegate. Weak.
  ToolbarActionViewController* viewController_;

  // Whether or not a popup is visible from a user action.
  bool user_shown_popup_visible_;

  // Whether or not a context menu is running (or is in the process of opening).
  bool contextMenuRunning_;

  base::WeakPtrFactory<ToolbarActionViewDelegateBridge> weakFactory_;

  DISALLOW_COPY_AND_ASSIGN(ToolbarActionViewDelegateBridge);
};

ToolbarActionViewDelegateBridge::ToolbarActionViewDelegateBridge(
    BrowserActionButton* owner,
    BrowserActionsController* controller,
    ToolbarActionViewController* viewController)
    : owner_(owner),
      controller_(controller),
      viewController_(viewController),
      user_shown_popup_visible_(false),
      contextMenuRunning_(false),
      weakFactory_(this) {
  viewController_->SetDelegate(this);
}

ToolbarActionViewDelegateBridge::~ToolbarActionViewDelegateBridge() {
  viewController_->SetDelegate(nullptr);
}

void ToolbarActionViewDelegateBridge::ShowContextMenu() {
  DCHECK(![controller_ toolbarActionsBar]->in_overflow_mode());
  if ([owner_ superview]) {
    // If the button is already visible on the toolbar, we can skip ahead to
    // just showing the menu.
    DoShowContextMenu();
    return;
  }

  // Otherwise, we have to slide the button out.
  contextMenuRunning_ = true;
  AppMenuController* appMenuController =
      [[[BrowserWindowController browserWindowControllerForWindow:
          [controller_ browser]->window()->GetNativeWindow()]
              toolbarController] appMenuController];
  // If the app menu is open, we have to first close it. Part of this happens
  // asynchronously, so we have to use a posted task to open the next menu.
  if ([appMenuController isMenuOpen])
    [appMenuController cancel];

  [controller_ toolbarActionsBar]->PopOutAction(
      viewController_,
      false,
      base::Bind(&ToolbarActionViewDelegateBridge::DoShowContextMenu,
                 weakFactory_.GetWeakPtr()));
}

content::WebContents* ToolbarActionViewDelegateBridge::GetCurrentWebContents()
    const {
  return [controller_ currentWebContents];
}

void ToolbarActionViewDelegateBridge::UpdateState() {
  [owner_ updateState];
}

bool ToolbarActionViewDelegateBridge::IsMenuRunning() const {
  MenuControllerCocoa* menuController = [owner_ contextMenuController];
  return contextMenuRunning_ || (menuController && [menuController isMenuOpen]);
}

void ToolbarActionViewDelegateBridge::OnPopupShown(bool by_user) {
  if (by_user)
    user_shown_popup_visible_ = true;
  [owner_ updateHighlightedState];
}

void ToolbarActionViewDelegateBridge::OnPopupClosed() {
  popup_closed_time_ = base::TimeTicks::Now();
  user_shown_popup_visible_ = false;
  [owner_ updateHighlightedState];
}

void ToolbarActionViewDelegateBridge::DoShowContextMenu() {
  // The point the menu shows matches that of the normal app menu - that is, the
  // right-left most corner of the menu is left-aligned with the app button,
  // and the menu is displayed "a little bit" lower. It would be nice to be able
  // to avoid the magic '5' here, but since it's built into Cocoa, it's not too
  // hopeful.
  NSPoint menuPoint = NSMakePoint(0, NSHeight([owner_ bounds]) + 5);
  base::WeakPtr<ToolbarActionViewDelegateBridge> weakThis =
      weakFactory_.GetWeakPtr();
  [[owner_ cell] setHighlighted:YES];
  [[owner_ menu] popUpMenuPositioningItem:nil
                               atLocation:menuPoint
                                   inView:owner_];
  // Since menus run in a blocking way, it's possible that the extension was
  // unloaded since this point.
  if (!weakThis)
    return;
  [[owner_ cell] setHighlighted:NO];
  contextMenuRunning_ = false;
}

@implementation BrowserActionButton

@synthesize isBeingDragged = isBeingDragged_;

+ (Class)cellClass {
  return [BrowserActionCell class];
}

- (id)initWithFrame:(NSRect)frame
     viewController:(ToolbarActionViewController*)viewController
         controller:(BrowserActionsController*)controller {
  if ((self = [super initWithFrame:frame])) {
    BrowserActionCell* cell = [[[BrowserActionCell alloc] init] autorelease];
    // [NSButton setCell:] warns to NOT use setCell: other than in the
    // initializer of a control.  However, we are using a basic
    // NSButton whose initializer does not take an NSCell as an
    // object.  To honor the assumed semantics, we do nothing with
    // NSButton between alloc/init and setCell:.
    [self setCell:cell];

    browserActionsController_ = controller;
    viewController_ = viewController;
    viewControllerDelegate_.reset(
        new ToolbarActionViewDelegateBridge(self, controller, viewController));

    [cell setBrowserActionsController:controller];
    [cell
        accessibilitySetOverrideValue:base::SysUTF16ToNSString(
            viewController_->GetAccessibleName([controller currentWebContents]))
        forAttribute:NSAccessibilityDescriptionAttribute];
    [self setTitle:@""];
    [self setButtonType:NSMomentaryChangeButton];
    [self setShowsBorderOnlyWhileMouseInside:YES];

    moveAnimation_.reset([[NSViewAnimation alloc] init]);
    [moveAnimation_ gtm_setDuration:kAnimationDuration
                          eventMask:NSLeftMouseUpMask];
    [moveAnimation_ setAnimationBlockingMode:NSAnimationNonblocking];

    [self updateState];
  }

  return self;
}

- (BOOL)acceptsFirstResponder {
  return YES;
}

- (void)rightMouseDown:(NSEvent*)theEvent {
  // Cocoa doesn't allow menus-running-in-menus, so in order to show the
  // context menu for an overflowed action, we close the app menu and show the
  // context menu over the app menu (similar to what we do for popups).
  // Let the main bar's button handle showing the context menu, since the app
  // menu will close..
  if ([browserActionsController_ isOverflow]) {
    [browserActionsController_ mainButtonForId:viewController_->GetId()]->
        viewControllerDelegate_->ShowContextMenu();
  } else {
    [super rightMouseDown:theEvent];
  }
}

- (void)mouseDown:(NSEvent*)theEvent {
  NSPoint location = [self convertPoint:[theEvent locationInWindow]
                               fromView:nil];
  // We don't allow dragging in the overflow container because mouse events
  // don't work well in menus in Cocoa. Specifically, the minute the mouse
  // leaves the view, the view stops receiving events. This is bad, because the
  // mouse can leave the view in many ways (user moves the mouse fast, user
  // tries to drag the icon to a non-applicable place, like outside the menu,
  // etc). When the mouse leaves, we get no indication (no mouseUp), so we can't
  // even handle that case - and are left in the middle of a drag. Instead, we
  // have to simply disable dragging.
  //
  // NOTE(devlin): If we use a greedy event loop that consumes all incoming
  // events (i.e. using [NSWindow nextEventMatchingMask]), we can make this
  // work. The downside to that is that all other events are lost. Disable this
  // for now, and revisit it at a later date.

  if (NSPointInRect(location, [self bounds]) &&
      ![browserActionsController_ isOverflow]) {
    dragCouldStart_ = YES;
    dragStartPoint_ = [self convertPoint:[theEvent locationInWindow]
                                fromView:nil];
    [self updateHighlightedState];
  }
}

- (void)mouseDragged:(NSEvent*)theEvent {
  if (!dragCouldStart_)
    return;

  NSPoint eventPoint = [theEvent locationInWindow];
  if (!isBeingDragged_) {
    // Don't initiate a drag until it moves at least kMinimumDragDistance.
    NSPoint dragStart = [self convertPoint:dragStartPoint_ toView:nil];
    CGFloat dx = eventPoint.x - dragStart.x;
    CGFloat dy = eventPoint.y - dragStart.y;
    if (dx*dx + dy*dy < kMinimumDragDistance*kMinimumDragDistance)
      return;

    // The start of a drag. Position the button above all others.
    [[self superview] addSubview:self positioned:NSWindowAbove relativeTo:nil];

    // We reset the |dragStartPoint_| so that the mouse can always be in the
    // same point along the button's x axis, and we avoid a "jump" when first
    // starting to drag.
    dragStartPoint_ = [self convertPoint:eventPoint fromView:nil];

    isBeingDragged_ = YES;
  }

  NSRect buttonFrame = [self frame];
  // The desired x is the current mouse point, minus the original offset of the
  // mouse into the button.
  NSPoint localPoint = [[self superview] convertPoint:eventPoint fromView:nil];
  CGFloat desiredX = localPoint.x - dragStartPoint_.x;
  // Clamp the button to be within its superview along the X-axis.
  NSRect containerBounds = [[self superview] bounds];
  desiredX = std::min(std::max(NSMinX(containerBounds), desiredX),
                      NSMaxX(containerBounds) - NSWidth(buttonFrame));
  buttonFrame.origin.x = desiredX;

  // If the button is in the overflow menu, it could move along the y-axis, too.
  if ([browserActionsController_ isOverflow]) {
    CGFloat desiredY = localPoint.y - dragStartPoint_.y;
    desiredY = std::min(std::max(NSMinY(containerBounds), desiredY),
                        NSMaxY(containerBounds) - NSHeight(buttonFrame));
    buttonFrame.origin.y = desiredY;
  }

  [self setFrame:buttonFrame];
  [self setNeedsDisplay:YES];
  [[NSNotificationCenter defaultCenter]
      postNotificationName:kBrowserActionButtonDraggingNotification
      object:self];
}

- (void)mouseUp:(NSEvent*)theEvent {
  dragCouldStart_ = NO;
  // There are non-drag cases where a mouseUp: may happen
  // (e.g. mouse-down, cmd-tab to another application, move mouse,
  // mouse-up).
  NSPoint location = [self convertPoint:[theEvent locationInWindow]
                               fromView:nil];
  // Only perform the click if we didn't drag the button.
  if (NSPointInRect(location, [self bounds]) && !isBeingDragged_) {
    if (viewControllerDelegate_->CanShowPopup()) {
      [self performClick:self];
    }
  } else {
    // Make sure an ESC to end a drag doesn't trigger 2 endDrags.
    if (isBeingDragged_) {
      [self endDrag];
    } else {
      [super mouseUp:theEvent];
    }
  }
  [self updateHighlightedState];
}

- (void)endDrag {
  isBeingDragged_ = NO;
  [[NSNotificationCenter defaultCenter]
      postNotificationName:kBrowserActionButtonDragEndNotification object:self];
}

- (void)updateHighlightedState {
  // The button's cell is highlighted if either the popup is showing by a user
  // action, or the user is about to drag the button, unless the button is
  // overflowed (in which case it is never highlighted).
  if ([self superview] && ![browserActionsController_ isOverflow]) {
    BOOL highlighted = viewControllerDelegate_->user_shown_popup_visible() ||
        dragCouldStart_;
    [[self cell] setHighlighted:highlighted];
  } else {
    [[self cell] setHighlighted:NO];
  }
}

- (MenuControllerCocoa*)contextMenuController {
  return contextMenuController_.get();
}

- (void)menuDidClose:(NSNotification*)notification {
  viewController_->OnContextMenuClosed();
}

- (void)setFrame:(NSRect)frameRect animate:(BOOL)animate {
  if (!animate) {
    [self setFrame:frameRect];
  } else {
    if ([moveAnimation_ isAnimating])
      [moveAnimation_ stopAnimation];

    NSDictionary* animationDictionary = @{
      NSViewAnimationTargetKey : self,
      NSViewAnimationStartFrameKey : [NSValue valueWithRect:[self frame]],
      NSViewAnimationEndFrameKey : [NSValue valueWithRect:frameRect]
    };
    [moveAnimation_ setViewAnimations: @[ animationDictionary ]];
    [moveAnimation_ startAnimation];
  }
}

- (void)updateState {
  content::WebContents* webContents =
      [browserActionsController_ currentWebContents];
  if (!webContents)
    return;

  base::string16 tooltip = viewController_->GetTooltip(webContents);
  [self setToolTip:(tooltip.empty() ? nil : base::SysUTF16ToNSString(tooltip))];

  // For now, on Cocoa, pretend that the button is always in the normal state.
  // This only affects behavior when
  // extensions::features::kRuntimeHostPermissions is enabled (which is
  // currently off everywhere by default), and there's a good likelihood that
  // MacViews for the toolbar (https://crbug.com/671916) may ship prior to this.
  // Since it's a non-trivial amount of work to get this right in Cocoa, settle
  // with this for now. (The only behavior difference is that the icon
  // background won't darken on mouseover and click).
  gfx::Image image =
      viewController_->GetIcon(webContents, gfx::Size([self frame].size),
                               ToolbarActionButtonState::kNormal);

  if (!image.IsEmpty())
    [self setImage:image.ToNSImage()];

  BOOL enabled = viewController_->IsEnabled(webContents) ||
                 viewController_->DisabledClickOpensMenu();
  [self setEnabled:enabled];

  [self setNeedsDisplay:YES];
}

- (void)onRemoved {
  // The button is being removed from the toolbar, and the backing controller
  // will also be removed. Destroy the delegate.
  // We only need to do this because in Cocoa's memory management, removing the
  // button from the toolbar doesn't synchronously dealloc it.
  viewControllerDelegate_.reset();
  // Also reset the context menu, since it has a dependency on the backing
  // controller (which owns its model).
  contextMenuController_.reset();
  // Remove any lingering observations.
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (BOOL)isAnimating {
  return [moveAnimation_ isAnimating];
}

- (void)stopAnimation {
  // Stopping an in-progress NSViewAnimation sets the view's frame to the end
  // frame of the animation. We want animation to stop in-place, so re-set the
  // frame to what it is currently.
  NSRect frame = [self frame];
  if ([moveAnimation_ isAnimating])
    [moveAnimation_ stopAnimation];
  [self setFrame:frame];
}

- (NSRect)frameAfterAnimation {
  if ([moveAnimation_ isAnimating]) {
    NSRect endFrame = [[[[moveAnimation_ viewAnimations] objectAtIndex:0]
        valueForKey:NSViewAnimationEndFrameKey] rectValue];
    return endFrame;
  } else {
    return [self frame];
  }
}

- (ToolbarActionViewController*)viewController {
  return viewController_;
}

- (NSImage*)compositedImage {
  NSRect bounds = [self bounds];
  NSImage* image = [[[NSImage alloc] initWithSize:bounds.size] autorelease];
  [image lockFocus];

  [[NSColor clearColor] set];
  NSRectFill(bounds);

  NSImage* actionImage = [self image];
  const NSSize imageSize = [actionImage size];
  const NSRect imageRect =
      NSMakeRect(std::floor((NSWidth(bounds) - imageSize.width) / 2.0),
                 std::floor((NSHeight(bounds) - imageSize.height) / 2.0),
                 imageSize.width, imageSize.height);
  [actionImage drawInRect:imageRect
                 fromRect:NSZeroRect
                operation:NSCompositeSourceOver
                 fraction:1.0
           respectFlipped:YES
                    hints:nil];

  [image unlockFocus];
  return image;
}

- (void)showContextMenu {
  viewControllerDelegate_->ShowContextMenu();
}

- (NSMenu*)menu {
  // Hack: Since Cocoa doesn't support menus-running-in-menus (see also comment
  // in -rightMouseDown:), it doesn't launch the menu for an overflowed action
  // on a Control-click. Even more unfortunate, it doesn't even pass us the
  // mouseDown event for control clicks. However, it does call -menuForEvent:,
  // which in turn calls -menu:, so we can tap in here and show the menu
  // programmatically for the Control-click case.
  if ([browserActionsController_ isOverflow] &&
      ([NSEvent modifierFlags] & NSControlKeyMask)) {
    [browserActionsController_ mainButtonForId:viewController_->GetId()]->
        viewControllerDelegate_->ShowContextMenu();
    return nil;
  }

  NSMenu* menu = nil;
  if (testContextMenu_) {
    menu = testContextMenu_;
  } else {
    // Make sure we delete any references to an old menu.
    contextMenuController_.reset();

    ui::MenuModel* contextMenu = viewController_->GetContextMenu();
    if (contextMenu) {
      contextMenuController_.reset([[MenuControllerCocoa alloc]
                   initWithModel:contextMenu
          useWithPopUpButtonCell:NO]);
      menu = [contextMenuController_ menu];
    }
  }

  if (menu) {
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(menuDidClose:)
               name:NSMenuDidEndTrackingNotification
             object:menu];
  }
  return menu;
}

#pragma mark -
#pragma mark Testing Methods

- (void)setTestContextMenu:(NSMenu*)testContextMenu {
  testContextMenu_ = testContextMenu;
}

- (BOOL)wantsToRunForTesting {
  return viewController_->WantsToRun(
      [browserActionsController_ currentWebContents]);
}

- (BOOL)isHighlighted {
  return [[self cell] isHighlighted];
}

@end

@implementation BrowserActionCell

@synthesize browserActionsController = browserActionsController_;

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView*)controlView {
  gfx::ScopedNSGraphicsContextSaveGState scopedGState;
  [super drawWithFrame:cellFrame inView:controlView];

  const NSSize imageSize = self.image.size;
  const NSRect imageRect =
      NSMakeRect(std::floor((NSWidth(cellFrame) - imageSize.width) / 2.0),
                 std::floor((NSHeight(cellFrame) - imageSize.height) / 2.0),
                 imageSize.width, imageSize.height);

  [self.image drawInRect:imageRect
                fromRect:NSZeroRect
               operation:NSCompositeSourceOver
                fraction:1.0
          respectFlipped:YES
                   hints:nil];
}

- (void)drawFocusRingMaskWithFrame:(NSRect)cellFrame inView:(NSView*)view {
  // Match the hover image's bezel.
  [[NSBezierPath bezierPathWithRoundedRect:NSInsetRect(cellFrame, 2, 2)
                                   xRadius:2
                                   yRadius:2] fill];
}

- (const ui::ThemeProvider*)themeProviderForWindow:(NSWindow*)window {
  const ui::ThemeProvider* themeProvider = [window themeProvider];
  if (!themeProvider)
    themeProvider =
        [[browserActionsController_ browser]->window()->GetNativeWindow()
            themeProvider];
  return themeProvider;
}

@end
