// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/toolbar/reload_button_cocoa.h"

#include <stddef.h>

#include "base/macros.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/command_updater.h"
#import "chrome/browser/ui/cocoa/accelerators_cocoa.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#import "chrome/browser/ui/cocoa/view_id_util.h"
#include "chrome/grit/generated_resources.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/accelerators/platform_accelerator_cocoa.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#import "ui/events/event_utils.h"

namespace {

// Constant matches Windows.
NSTimeInterval kPendingReloadTimeout = 1.35;

// Contents of the Reload drop-down menu.
const int kReloadMenuItems[]  = {
  IDS_RELOAD_MENU_NORMAL_RELOAD_ITEM,
  IDS_RELOAD_MENU_HARD_RELOAD_ITEM,
  IDS_RELOAD_MENU_EMPTY_AND_HARD_RELOAD_ITEM,
};
// Note: must have the same size as |kReloadMenuItems|.
const int kReloadMenuCommands[]  = {
  IDC_RELOAD,
  IDC_RELOAD_BYPASSING_CACHE,
  IDC_RELOAD_CLEARING_CACHE,
};

}  // namespace

@interface ReloadButton ()
- (void)invalidatePendingReloadTimer;
- (void)forceReloadState:(NSTimer *)timer;
- (void)populateMenu;
@end

@implementation ReloadButton

+ (Class)cellClass {
  return [ClickHoldButtonCell class];
}

- (id)initWithFrame:(NSRect)frameRect {
  if ((self = [super initWithFrame:frameRect])) {
    // Since this is not a custom view, -awakeFromNib won't be called twice.
    [self awakeFromNib];
  }
  return self;
}

- (void)viewWillMoveToWindow:(NSWindow *)newWindow {
  // If this view is moved to a new window, reset its state.
  [self setIsLoading:NO force:YES];
  [super viewWillMoveToWindow:newWindow];
}

- (void)awakeFromNib {
  // Don't allow multi-clicks, because the user probably wouldn't ever
  // want to stop+reload or reload+stop.
  [self setIgnoresMultiClick:YES];

  [self setOpenMenuOnRightClick:YES];
  [self setOpenMenuOnClick:NO];

  menu_.reset([[NSMenu alloc] initWithTitle:@""]);
  [self populateMenu];
  [self setAttachedMenu:menu_];
}

- (BOOL)shouldMirrorInRTL {
  return NO;
}

- (void)invalidatePendingReloadTimer {
  [pendingReloadTimer_ invalidate];
  pendingReloadTimer_ = nil;
}

- (void)updateTag:(NSInteger)anInt {
  if ([self tag] == anInt)
    return;

  // Forcibly remove any stale tooltip which is being displayed.
  [self removeAllToolTips];
  [self setTag:anInt];

  [self resetButtonStateImages];
  if (anInt == IDC_RELOAD) {
    [self setToolTip:l10n_util::GetNSStringWithFixup(IDS_TOOLTIP_RELOAD)];
  } else if (anInt == IDC_STOP) {
    [self setToolTip:l10n_util::GetNSStringWithFixup(IDS_TOOLTIP_STOP)];
  } else {
    NOTREACHED();
  }
}

- (id)accessibilityAttributeValue:(NSString *)attribute {
  if ([attribute isEqualToString:NSAccessibilityEnabledAttribute] &&
      pendingReloadTimer_) {
    return [NSNumber numberWithBool:NO];
  } else {
    return [super accessibilityAttributeValue:attribute];
  }
}

- (void)setIsLoading:(BOOL)isLoading force:(BOOL)force {
  // Can always transition to stop mode.  Only transition to reload
  // mode if forced or if the mouse isn't hovering.  Otherwise, note
  // that reload mode is desired and disable the button.
  if (isLoading) {
    [self invalidatePendingReloadTimer];
    [self updateTag:IDC_STOP];
  } else if (force) {
    [self invalidatePendingReloadTimer];
    [self updateTag:IDC_RELOAD];
  } else if ([self tag] == IDC_STOP &&
             !pendingReloadTimer_ &&
             [[self cell] isMouseInside]) {
    [self resetButtonStateImages];
    NSImage* disabledStopImage =
        [[self cell] imageForState:image_button_cell::kDisabledState
                              view:self];
    [[self cell] setImage:disabledStopImage
        forButtonState:image_button_cell::kDefaultState];
    [[self cell] setImage:disabledStopImage
        forButtonState:image_button_cell::kHoverState];
    [[self cell] setImage:disabledStopImage
        forButtonState:image_button_cell::kPressedState];
    pendingReloadTimer_ =
        [NSTimer timerWithTimeInterval:kPendingReloadTimeout
                                target:self
                              selector:@selector(forceReloadState:)
                              userInfo:nil
                               repeats:NO];
    // Must add the timer to |NSRunLoopCommonModes| because
    // it should run in |NSEventTrackingRunLoopMode| as well as
    // |NSDefaultRunLoopMode|.
    [[NSRunLoop currentRunLoop] addTimer:pendingReloadTimer_
                                 forMode:NSRunLoopCommonModes];
  } else {
    [self invalidatePendingReloadTimer];
    [self updateTag:IDC_RELOAD];
  }
  [self setEnabled:pendingReloadTimer_ == nil];
}

- (void)setMenuEnabled:(BOOL)enabled {
  [self setOpenMenuOnRightClick:enabled];
  [self setOpenMenuOnClickHold:enabled];
}

- (void)setCommandUpdater:(CommandUpdater*)commandUpdater {
  commandUpdater_ = commandUpdater;
}

- (void)forceReloadState:(NSTimer *)timer {
  DCHECK_EQ(timer, pendingReloadTimer_);
  [self setIsLoading:NO force:YES];
  // Verify that |pendingReloadTimer_| is nil so it is not left dangling.
  DCHECK(!pendingReloadTimer_);
}

- (BOOL)sendAction:(SEL)theAction to:(id)theTarget {
  if ([self tag] == IDC_STOP) {
    if (pendingReloadTimer_) {
      // If |pendingReloadTimer_| then the control is currently being
      // drawn in a disabled state, so just return. The control is NOT actually
      // disabled, otherwise mousetracking (courtesy of the NSButtonCell)
      // would not work.
      return YES;
    } else {
      // When the stop is processed, immediately change to reload mode,
      // even though the IPC still has to bounce off the renderer and
      // back before the regular |-setIsLoaded:force:| will be called.
      // [This is how views and gtk do it.]
      BOOL ret = [super sendAction:theAction to:theTarget];
      if (ret)
        [self forceReloadState:pendingReloadTimer_];
      return ret;
    }
  }

  return [super sendAction:theAction to:theTarget];
}

- (ViewID)viewID {
  return VIEW_ID_RELOAD_BUTTON;
}

- (const gfx::VectorIcon*)vectorIcon {
  if ([self tag] == IDC_RELOAD) {
    return &vector_icons::kReloadIcon;
  } else if ([self tag] == IDC_STOP) {
    return &kNavigateStopIcon;
  } else {
    NOTREACHED();
  }

  return nullptr;
}

- (void)mouseInsideStateDidChange:(BOOL)isInside {
  [pendingReloadTimer_ fire];
}

- (void)populateMenu {
  [menu_ setAutoenablesItems:NO];
  // 0-th item must be blank. (This is because we use a pulldown list, for which
  // Cocoa uses the 0-th item as "title" in the button.)
  [menu_ addItemWithTitle:@""
                   action:nil
            keyEquivalent:@""];
  AcceleratorsCocoa* keymap = AcceleratorsCocoa::GetInstance();
  for (size_t i = 0; i < arraysize(kReloadMenuItems); ++i) {
    NSString* title = l10n_util::GetNSStringWithFixup(kReloadMenuItems[i]);
    base::scoped_nsobject<NSMenuItem> item(
        [[NSMenuItem alloc] initWithTitle:title
                                   action:@selector(executeMenuItem:)
                            keyEquivalent:@""]);

    const ui::Accelerator* accelerator =
        keymap->GetAcceleratorForCommand(kReloadMenuCommands[i]);
    if (accelerator) {
      const ui::PlatformAcceleratorCocoa* platform =
          static_cast<const ui::PlatformAcceleratorCocoa*>(
                accelerator->platform_accelerator());
      if (platform) {
        [item setKeyEquivalent:platform->characters()];
        [item setKeyEquivalentModifierMask:platform->modifier_mask()];
      }
    }

    [item setTag:kReloadMenuCommands[i]];
    [item setTarget:self];
    [item setEnabled:YES];

    [menu_ addItem:item];
  }
}

// Action for menu items.
- (void)executeMenuItem:(id)sender {
  if (!commandUpdater_)
    return;
  DCHECK([sender isKindOfClass:[NSMenuItem class]]);
  int command = [sender tag];
  int event_flags = ui::EventFlagsFromNative([NSApp currentEvent]);
  commandUpdater_->ExecuteCommandWithDisposition(
      command, ui::DispositionFromEventFlags(event_flags));
}

@end  // ReloadButton

@implementation ReloadButton (Testing)

+ (void)setPendingReloadTimeout:(NSTimeInterval)seconds {
  kPendingReloadTimeout = seconds;
}

@end
