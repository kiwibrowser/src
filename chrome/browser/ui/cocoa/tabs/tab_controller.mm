// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tabs/tab_controller.h"

#import <QuartzCore/QuartzCore.h>

#include <algorithm>
#include <cmath>

#include "base/i18n/rtl.h"
#include "base/mac/bundle_locations.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/themes/theme_properties.h"
#import "chrome/browser/themes/theme_service.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/tabs/alert_indicator_button_cocoa.h"
#import "chrome/browser/ui/cocoa/tabs/tab_controller_target.h"
#include "chrome/browser/ui/cocoa/tabs/tab_favicon_view.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_view.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#import "extensions/common/extension.h"
#include "skia/ext/skia_utils_mac.h"
#import "ui/base/cocoa/menu_controller.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/favicon_size.h"
#include "ui/native_theme/native_theme.h"

namespace {

// A C++ delegate that handles enabling/disabling menu items and handling when
// a menu command is chosen. Also fixes up the menu item label for "pin/unpin
// tab".
class MenuDelegate : public ui::SimpleMenuModel::Delegate {
 public:
  explicit MenuDelegate(id<TabControllerTarget> target, TabController* owner)
      : target_(target),
        owner_(owner) {}

  // Overridden from ui::SimpleMenuModel::Delegate
  bool IsCommandIdChecked(int command_id) const override { return false; }
  bool IsCommandIdEnabled(int command_id) const override {
    TabStripModel::ContextMenuCommand command =
        static_cast<TabStripModel::ContextMenuCommand>(command_id);
    return [target_ isCommandEnabled:command forController:owner_];
  }
  void ExecuteCommand(int command_id, int event_flags) override {
    TabStripModel::ContextMenuCommand command =
        static_cast<TabStripModel::ContextMenuCommand>(command_id);
    [target_ commandDispatch:command forController:owner_];
  }

 private:
  id<TabControllerTarget> target_;  // weak
  TabController* owner_;  // weak, owns me
};

}  // namespace

@interface TabController () {
  base::scoped_nsobject<TabFaviconView> iconView_;
  base::scoped_nsobject<NSImage> icon_;
  base::scoped_nsobject<NSView> attentionDotView_;
  base::scoped_nsobject<AlertIndicatorButton> alertIndicatorButton_;
  base::scoped_nsobject<HoverCloseButton> closeButton_;

  BOOL active_;
  BOOL selected_;
  std::unique_ptr<ui::SimpleMenuModel> contextMenuModel_;
  std::unique_ptr<MenuDelegate> contextMenuDelegate_;
  base::scoped_nsobject<MenuControllerCocoa> contextMenuController_;

  enum AttentionType : int {
    kBlockedWebContents = 1 << 0,       // The WebContents is marked as blocked.
    kTabWantsAttentionStatus = 1 << 1,  // SetTabNeedsAttention() was called.
  };
}

@property(nonatomic) int currentAttentionTypes;  // Bitmask of AttentionType.

// Recomputes the iconView's frame and updates it with or without animation.
- (void)updateIconViewFrameWithAnimation:(BOOL)shouldAnimate;

@end

@implementation TabController

@synthesize action = action_;
@synthesize currentAttentionTypes = currentAttentionTypes_;
@synthesize loadingState = loadingState_;
@synthesize showIcon = showIcon_;
@synthesize pinned = pinned_;
@synthesize target = target_;
@synthesize url = url_;

namespace {
constexpr CGFloat kTabLeadingPadding = 18;
constexpr CGFloat kTabTrailingPadding = 15;
constexpr CGFloat kMinTabWidth = 36;
constexpr CGFloat kMinActiveTabWidth = 52;
constexpr CGFloat kMaxTabWidth = 246;
constexpr CGFloat kCloseButtonSize = 16;
constexpr CGFloat kInitialTabWidth = 160;
constexpr CGFloat kTitleLeadingPadding = 4;
constexpr CGFloat kInitialTitleWidth = 92;
constexpr CGFloat kTitleHeight = 17;
constexpr CGFloat kTabElementYOrigin = 6;
constexpr CGFloat kDefaultTabHeight = 29;
constexpr CGFloat kPinnedTabWidth = kDefaultTabHeight * 2;
}  // namespace

+ (CGFloat)defaultTabHeight {
  return kDefaultTabHeight;
}

// The min widths is the smallest number at which the right edge of the right
// tab border image is not visibly clipped.  It is a bit smaller than the sum
// of the two tab edge bitmaps because these bitmaps have a few transparent
// pixels on the side.  The selected tab width includes the close button width.
+ (CGFloat)minTabWidth {
  return kMinTabWidth;
}
+ (CGFloat)minActiveTabWidth {
  return kMinActiveTabWidth;
}
+ (CGFloat)maxTabWidth {
  return kMaxTabWidth;
}

+ (CGFloat)pinnedTabWidth {
  return kPinnedTabWidth;
}

- (TabView*)tabView {
  DCHECK([[self view] isKindOfClass:[TabView class]]);
  return static_cast<TabView*>([self view]);
}

- (id)init {
  if ((self = [super init])) {
    BOOL isRTL = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();

    // Create the close button.
    const CGFloat closeButtonXOrigin =
        isRTL ? kTabTrailingPadding
              : kInitialTabWidth - kCloseButtonSize - kTabTrailingPadding;
    NSRect closeButtonFrame = NSMakeRect(closeButtonXOrigin, kTabElementYOrigin,
                                         kCloseButtonSize, kCloseButtonSize);
    closeButton_.reset(
        [[HoverCloseButton alloc] initWithFrame:closeButtonFrame]);
    [closeButton_
        setAutoresizingMask:isRTL ? NSViewMaxXMargin : NSViewMinXMargin];
    [closeButton_ setTarget:self];
    [closeButton_ setAction:@selector(closeTab:)];

    // Create the TabView. The TabView works directly with the closeButton so
    // here (the TabView handles adding it as a subview).
    base::scoped_nsobject<TabView> tabView([[TabView alloc]
        initWithFrame:NSMakeRect(0, 0, kInitialTabWidth,
                                 [TabController defaultTabHeight])
           controller:self
          closeButton:closeButton_]);
    [tabView setAutoresizingMask:NSViewMaxXMargin | NSViewMinYMargin];
    [tabView setPostsFrameChangedNotifications:NO];
    [tabView setPostsBoundsChangedNotifications:NO];
    [super setView:tabView];

    // Add the favicon view.
    NSRect iconViewFrame =
        NSMakeRect(0, kTabElementYOrigin, gfx::kFaviconSize, gfx::kFaviconSize);
    iconView_.reset([[TabFaviconView alloc] initWithFrame:iconViewFrame]);
    [iconView_ setAutoresizingMask:isRTL ? NSViewMinXMargin | NSViewMinYMargin
                                         : NSViewMaxXMargin | NSViewMinYMargin];
    [self updateIconViewFrameWithAnimation:NO];
    [tabView addSubview:iconView_];

    // Set up the title.
    const CGFloat titleXOrigin =
        isRTL ? NSMinX([iconView_ frame]) - kTitleLeadingPadding -
                    kInitialTitleWidth
              : NSMaxX([iconView_ frame]) + kTitleLeadingPadding;
    NSRect titleFrame = NSMakeRect(titleXOrigin, kTabElementYOrigin,
                                   kInitialTitleWidth, kTitleHeight);
    [tabView setTitleFrame:titleFrame];

    NSNotificationCenter* defaultCenter = [NSNotificationCenter defaultCenter];
    [defaultCenter addObserver:self
                      selector:@selector(themeChangedNotification:)
                          name:kBrowserThemeDidChangeNotification
                        object:nil];

    [self internalSetSelected:selected_];
  }
  return self;
}

- (void)dealloc {
  [alertIndicatorButton_ setAnimationDoneTarget:nil withAction:nil];
  [alertIndicatorButton_ setClickTarget:nil withAction:nil];
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [[self tabView] setController:nil];
  [super dealloc];
}

// The internals of |-setSelected:| and |-setActive:| but doesn't set the
// backing variables. This updates the drawing state and marks self as needing
// a re-draw.
- (void)internalSetSelected:(BOOL)selected {
  TabView* tabView = [self tabView];
  if ([self active])
    [tabView setState:NSOnState];
  else
    [tabView setState:selected ? NSMixedState : NSOffState];

  // The attention indicator must always be updated, as it needs to disappear
  // if a tab is blocked and is brought forward. It is updated at the end of
  // -updateVisibility.
  [self updateVisibility];
  [self updateTitleColor];
}

// Called when Cocoa wants to display the context menu. Lazily instantiate
// the menu based off of the cross-platform model. Re-create the menu and
// model every time to get the correct labels and enabling.
- (NSMenu*)menu {
  // If the menu is currently open, then this method is being called from
  // the nested runloop of the menu. This can happen when an accessibility
  // message is sent to retrieve the menu options. Do not delete the objects
  // associated with a running menu, which could lead to a use-after-free,
  // and instead just return the existing instance. https://crbug.com/778776
  if ([contextMenuController_ isMenuOpen]) {
    return [contextMenuController_ menu];
  }

  contextMenuDelegate_.reset(new MenuDelegate(target_, self));
  contextMenuModel_.reset(
      [target_ contextMenuModelForController:self
                                menuDelegate:contextMenuDelegate_.get()]);
  contextMenuController_.reset([[MenuControllerCocoa alloc]
               initWithModel:contextMenuModel_.get()
      useWithPopUpButtonCell:NO]);
  return [contextMenuController_ menu];
}

- (void)toggleMute:(id)sender {
  if ([[self target] respondsToSelector:@selector(toggleMute:)]) {
    [[self target] performSelector:@selector(toggleMute:)
                        withObject:[self view]];
  }
}

- (void)closeTab:(id)sender {
  using base::UserMetricsAction;

  if (alertIndicatorButton_ && ![alertIndicatorButton_ isHidden]) {
    if ([alertIndicatorButton_ isEnabled]) {
      base::RecordAction(UserMetricsAction("CloseTab_MuteToggleAvailable"));
    } else if ([alertIndicatorButton_ showingAlertState] ==
                   TabAlertState::AUDIO_PLAYING) {
      base::RecordAction(UserMetricsAction("CloseTab_AudioIndicator"));
    } else {
      base::RecordAction(UserMetricsAction("CloseTab_RecordingIndicator"));
    }
  } else {
    base::RecordAction(UserMetricsAction("CloseTab_NoAlertIndicator"));
  }

  if ([[self target] respondsToSelector:@selector(closeTab:)]) {
    [[self target] performSelector:@selector(closeTab:)
                        withObject:[self view]];
  }
}

- (void)selectTab:(id)sender {
  if ([[self tabView] isClosing])
    return;
  if ([[self target] respondsToSelector:[self action]]) {
    [[self target] performSelector:[self action]
                        withObject:[self view]];
  }
}

- (void)setTitle:(NSString*)title {
  if ([[self title] isEqualToString:title])
    return;

  TabView* tabView = [self tabView];
  [tabView setTitle:title];

  [super setTitle:title];
}

- (void)setActive:(BOOL)active {
  if (active != active_) {
    active_ = active;
    [self internalSetSelected:[self selected]];
  }
}

- (BOOL)active {
  return active_;
}

- (void)setSelected:(BOOL)selected {
  if (selected_ != selected) {
    selected_ = selected;
    [self internalSetSelected:[self selected]];
  }
}

- (BOOL)selected {
  return selected_ || active_;
}

- (void)setPinned:(BOOL)pinned {
  if (pinned_ != pinned) {
    pinned_ = pinned;
    [self updateIconViewFrameWithAnimation:YES];
  }
}

- (void)updateIconViewFrameWithAnimation:(BOOL)shouldAnimate {
  static const CGFloat kPinnedTabLeadingPadding =
      std::floor((kPinnedTabWidth - gfx::kFaviconSize) / 2.0);
  BOOL isRTL = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();

  // Determine the padding between the iconView and the tab edge.
  CGFloat leadingPadding =
      [self pinned] ? kPinnedTabLeadingPadding : kTabLeadingPadding;

  NSRect iconViewFrame = [iconView_ frame];
  iconViewFrame.origin.x = isRTL ? NSWidth([[self tabView] frame]) -
                                       leadingPadding - gfx::kFaviconSize
                                 : leadingPadding;

  // The iconView animation looks funky in RTL so don't allow it.
  if (shouldAnimate && !isRTL) {
    // Animate at the same rate as the tab changes shape.
    [[NSAnimationContext currentContext]
        setDuration:[TabStripController tabAnimationDuration]];
    [[iconView_ animator] setFrame:iconViewFrame];
  } else {
    [iconView_ setFrame:iconViewFrame];
  }
}

- (AlertIndicatorButton*)alertIndicatorButton {
  return alertIndicatorButton_;
}

- (void)setAlertState:(TabAlertState)alertState {
  if (!alertIndicatorButton_ && alertState != TabAlertState::NONE) {
    alertIndicatorButton_.reset([[AlertIndicatorButton alloc] init]);
    [self updateVisibility];  // Do layout and visibility before adding subview.
    [[self view] addSubview:alertIndicatorButton_];
    [alertIndicatorButton_ setAnimationDoneTarget:self
                                       withAction:@selector(updateVisibility)];
    [alertIndicatorButton_ setClickTarget:self
                               withAction:@selector(toggleMute:)];
  }
  [alertIndicatorButton_ transitionToAlertState:alertState];
}

- (BOOL)blocked {
  return self.currentAttentionTypes & AttentionType::kBlockedWebContents ? YES
                                                                         : NO;
}

- (void)setBlocked:(BOOL)blocked {
  if (blocked)
    self.currentAttentionTypes |= AttentionType::kBlockedWebContents;
  else
    self.currentAttentionTypes &= ~AttentionType::kBlockedWebContents;
}

- (void)setNeedsAttention:(bool)attention {
  if (attention)
    self.currentAttentionTypes |= AttentionType::kTabWantsAttentionStatus;
  else
    self.currentAttentionTypes &= ~AttentionType::kTabWantsAttentionStatus;
}

- (void)setCurrentAttentionTypes:(int)attentionTypes {
  if (currentAttentionTypes_ == attentionTypes)
    return;
  currentAttentionTypes_ = attentionTypes;
  [self updateAttentionIndicator];
}

- (HoverCloseButton*)closeButton {
  return closeButton_;
}

- (NSString*)toolTip {
  return [[self tabView] toolTipText];
}

- (void)setToolTip:(NSString*)toolTip {
  [[self tabView] setToolTipText:toolTip];
}

- (NSView*)iconView {
  return iconView_;
}

// Return a rough approximation of the number of icons we could fit in the
// tab. We never actually do this, but it's a helpful guide for determining
// how much space we have available.
- (int)iconCapacity {
  const CGFloat availableWidth =
      std::max<CGFloat>(0, NSWidth([[self tabView] frame]) -
                               kTabLeadingPadding - kTabTrailingPadding);
  const CGFloat widthPerIcon = gfx::kFaviconSize;
  const int kPaddingBetweenIcons = 2;
  if (availableWidth >= widthPerIcon &&
      availableWidth < (widthPerIcon + kPaddingBetweenIcons)) {
    return 1;
  }
  return availableWidth / (widthPerIcon + kPaddingBetweenIcons);
}

- (BOOL)shouldShowIcon {
  return chrome::ShouldTabShowFavicon(
      [self iconCapacity], [self pinned], [self active], [self showIcon],
      !alertIndicatorButton_ ? TabAlertState::NONE
                             : [alertIndicatorButton_ showingAlertState]);
}

- (BOOL)shouldShowAlertIndicator {
  return chrome::ShouldTabShowAlertIndicator(
      [self iconCapacity], [self pinned], [self active], [self showIcon],
      !alertIndicatorButton_ ? TabAlertState::NONE
                             : [alertIndicatorButton_ showingAlertState]);
}

- (BOOL)shouldShowCloseButton {
  return chrome::ShouldTabShowCloseButton(
      [self iconCapacity], [self pinned], [self active]);
}

- (void)setIconImage:(NSImage*)image
     forLoadingState:(TabLoadingState)newLoadingState
            showIcon:(BOOL)showIcon {
  // Update the favicon's visbility state. Note that TabStripController calls
  // -updateVisibility immediately after calling this method, so we don't need
  // to act on a change in this state.
  showIcon_ = showIcon;

  // Always draw the favicon when the state is already kTabDone because the site
  // may have sent an updated favicon.
  if (newLoadingState == loadingState_ && newLoadingState != kTabDone) {
    return;
  }
  loadingState_ = newLoadingState;

  if (newLoadingState == kTabDone) {
    [iconView_ setTabDoneStateWithIcon:image];
  } else {
    [iconView_ setTabLoadingState:newLoadingState];
  }
}

- (void)updateAttentionIndicator {
  // Don't show the attention indicator for blocked WebContentses if the tab is
  // active; it's distracting.
  int actualAttentionTypes = self.currentAttentionTypes;
  if ([self active])
    actualAttentionTypes &= ~AttentionType::kBlockedWebContents;

  if (actualAttentionTypes != 0 && ![iconView_ isHidden]) {
    // The attention indicator consists of two parts:
    // . a wedge cut out of the bottom right (or left in rtl) of the favicon.
    // . a circle in the bottom right (or left in rtl) of the favicon.
    //
    // The favicon lives in a view to itself, a view which is too small to
    // contain the dot (the second part of the indicator), so the dot is added
    // as a separate subview.
    BOOL isRTL = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();
    CGRect iconViewBounds = iconView_.get().layer.bounds;
    CGPoint indicatorCenter = CGPointMake(
        isRTL ? CGRectGetMinX(iconViewBounds) : CGRectGetMaxX(iconViewBounds),
        CGRectGetMinY(iconViewBounds));

    const CGFloat kIndicatorCropRadius = 4.5;
    CGRect cropCircleBounds = CGRectZero;
    cropCircleBounds.origin = indicatorCenter;
    cropCircleBounds = CGRectInset(cropCircleBounds, -kIndicatorCropRadius,
                                   -kIndicatorCropRadius);

    base::ScopedCFTypeRef<CGMutablePathRef> maskPath(CGPathCreateMutable());
    CGPathAddRect(maskPath, nil, iconViewBounds);
    CGPathAddEllipseInRect(maskPath, nil, cropCircleBounds);

    CAShapeLayer* maskLayer = [CAShapeLayer layer];
    maskLayer.frame = iconViewBounds;
    maskLayer.path = maskPath.get();
    maskLayer.fillRule = kCAFillRuleEvenOdd;
    iconView_.get().layer.mask = maskLayer;

    if (!attentionDotView_) {
      NSRect iconViewFrame = [iconView_ frame];
      NSPoint indicatorCenter =
          NSMakePoint(isRTL ? NSMinX(iconViewFrame) : NSMaxX(iconViewFrame),
                      NSMinY(iconViewFrame));

      const float kIndicatorRadius = 3.0f;
      NSRect indicatorCircleFrame = NSZeroRect;
      indicatorCircleFrame.origin = indicatorCenter;
      indicatorCircleFrame = NSInsetRect(indicatorCircleFrame,
                                         -kIndicatorRadius, -kIndicatorRadius);
      attentionDotView_.reset(
          [[NSView alloc] initWithFrame:indicatorCircleFrame]);
      attentionDotView_.get().wantsLayer = YES;
      SkColor indicatorColor =
          ui::NativeTheme::GetInstanceForNativeUi()->GetSystemColor(
              ui::NativeTheme::kColorId_ProminentButtonColor);
      attentionDotView_.get().layer.backgroundColor =
          skia::SkColorToSRGBNSColor(indicatorColor).CGColor;
      attentionDotView_.get().layer.cornerRadius = kIndicatorRadius;

      [[self view] addSubview:attentionDotView_];
    }
  } else {
    iconView_.get().layer.mask = nil;
    [attentionDotView_ removeFromSuperview];
    attentionDotView_.reset();
  }
}

- (void)updateVisibility {
  BOOL newShowIcon = [self shouldShowIcon];

  [iconView_ setHidden:!newShowIcon];

  // If the tab is a pinned-tab, hide the title.
  TabView* tabView = [self tabView];
  [tabView setTitleHidden:[self pinned]];

  BOOL newShowCloseButton = [self shouldShowCloseButton];

  [closeButton_ setHidden:!newShowCloseButton];

  BOOL newShowAlertIndicator = [self shouldShowAlertIndicator];

  [alertIndicatorButton_ setHidden:!newShowAlertIndicator];

  BOOL isRTL = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();

  if (newShowAlertIndicator) {
    NSRect newFrame = [alertIndicatorButton_ frame];
    newFrame.size = [[alertIndicatorButton_ image] size];
    if ([self pinned]) {
      // Tab is pinned: Position the alert indicator in the center.
      const CGFloat tabWidth = [TabController pinnedTabWidth];
      newFrame.origin.x = std::floor((tabWidth - NSWidth(newFrame)) / 2);
      newFrame.origin.y =
          kTabElementYOrigin -
          std::floor((NSHeight(newFrame) - gfx::kFaviconSize) / 2);
    } else {
      // The Frame for the alertIndicatorButton_ depends on whether iconView_
      // and/or closeButton_ are visible, and where they have been positioned.
      const NSRect closeButtonFrame = [closeButton_ frame];
      newFrame.origin.x = NSMinX(closeButtonFrame);
      // Position before the close button when it is showing.
      if (newShowCloseButton)
        newFrame.origin.x += isRTL ? NSWidth(newFrame) : -NSWidth(newFrame);
      // Alert indicator is centered vertically, with respect to closeButton_.
      newFrame.origin.y = NSMinY(closeButtonFrame) -
          std::floor((NSHeight(newFrame) - NSHeight(closeButtonFrame)) / 2);
    }
    [alertIndicatorButton_ setFrame:newFrame];
    [alertIndicatorButton_ updateEnabledForMuteToggle];
  }

  // Adjust the title view based on changes to the icon's and close button's
  // visibility.
  NSRect oldTitleFrame = [tabView titleFrame];
  NSRect newTitleFrame;
  newTitleFrame.size.height = oldTitleFrame.size.height;
  newTitleFrame.origin.y = oldTitleFrame.origin.y;

  CGFloat titleLeft, titleRight;
  if (isRTL) {
    if (newShowAlertIndicator) {
      titleLeft = NSMaxX([alertIndicatorButton_ frame]);
    } else if (newShowCloseButton) {
      titleLeft = NSMaxX([closeButton_ frame]);
    } else {
      titleLeft = kTabLeadingPadding;
    }
    titleRight = newShowIcon
                     ? NSMinX([iconView_ frame]) - kTitleLeadingPadding
                     : NSWidth([[self tabView] frame]) - kTabLeadingPadding;
  } else {
    titleLeft = newShowIcon ? NSMaxX([iconView_ frame]) + kTitleLeadingPadding
                            : kTabLeadingPadding;
    if (newShowAlertIndicator) {
      titleRight = NSMinX([alertIndicatorButton_ frame]);
    } else if (newShowCloseButton) {
      titleRight = NSMinX([closeButton_ frame]);
    } else {
      titleRight = NSWidth([[self tabView] frame]) - kTabTrailingPadding;
    }
  }

  newTitleFrame.size.width = titleRight - titleLeft;
  newTitleFrame.origin.x = titleLeft;

  [tabView setTitleFrame:newTitleFrame];

  [self updateAttentionIndicator];
}

- (void)updateTitleColor {
  NSColor* titleColor = nil;
  const ui::ThemeProvider* theme = [[[self view] window] themeProvider];
  if (theme && ![self selected])
    titleColor = theme->GetNSColor(ThemeProperties::COLOR_BACKGROUND_TAB_TEXT);
  // Default to the selected text color unless told otherwise.
  if (theme && !titleColor)
    titleColor = theme->GetNSColor(ThemeProperties::COLOR_TAB_TEXT);
  [[self tabView] setTitleColor:titleColor ? titleColor : [NSColor textColor]];
}

- (NSString*)accessibilityTitle {
  // TODO(ellyjones): the Cocoa tab strip code doesn't keep track of network
  // error state, so it can't get surfaced here. It should, and then this could
  // pass in the network error state.
  return base::SysUTF16ToNSString(chrome::AssembleTabAccessibilityLabel(
      base::SysNSStringToUTF16([self title]),
      [self loadingState] == kTabCrashed, false,
      [[self alertIndicatorButton] showingAlertState]));
}

- (void)themeChangedNotification:(NSNotification*)notification {
  [self updateTitleColor];
}

// Called by the tabs to determine whether we are in rapid (tab) closure mode.
- (BOOL)inRapidClosureMode {
  if ([[self target] respondsToSelector:@selector(inRapidClosureMode)]) {
    return [[self target] performSelector:@selector(inRapidClosureMode)] ?
        YES : NO;
  }
  return NO;
}

- (void)maybeStartDrag:(NSEvent*)event forTab:(TabController*)tab {
  [[target_ dragController] maybeStartDrag:event forTab:tab];
}

- (void)performClick:(id)sender {
  [self selectTab:self];
}

@end
