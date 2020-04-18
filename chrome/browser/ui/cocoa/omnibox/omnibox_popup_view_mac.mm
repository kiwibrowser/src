// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/omnibox/omnibox_popup_view_mac.h"

#include <cmath>

#include "base/mac/mac_util.h"
#import "base/mac/sdk_forward_declarations.h"
#include "base/stl_util.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/omnibox/omnibox_popup_cell.h"
#import "chrome/browser/ui/cocoa/omnibox/omnibox_popup_separator_view.h"
#include "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_match_type.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/omnibox/browser/omnibox_popup_model.h"
#include "components/toolbar/vector_icons.h"
#include "skia/ext/skia_utils_mac.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSAnimation+Duration.h"
#import "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/base/cocoa/flipped_view.h"
#include "ui/base/cocoa/window_size_constants.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"
#include "ui/gfx/text_elider.h"

namespace {

const int kMaterialPopupPaddingVertical = 4;

// Padding between matrix and the top and bottom of the popup window.
CGFloat PopupPaddingVertical() {
  return kMaterialPopupPaddingVertical;
}

// Animation duration when animating the popup window smaller.
const NSTimeInterval kShrinkAnimationDuration = 0.1;

}  // namespace

OmniboxPopupViewMac::OmniboxPopupViewMac(OmniboxView* omnibox_view,
                                         OmniboxEditModel* edit_model,
                                         NSTextField* field)
    : omnibox_view_(omnibox_view),
      model_(new OmniboxPopupModel(this, edit_model)),
      field_(field),
      popup_(nil) {
  DCHECK(omnibox_view);
  DCHECK(edit_model);
}

OmniboxPopupViewMac::~OmniboxPopupViewMac() {
  // Destroy the popup model before this object is destroyed, because
  // it can call back to us in the destructor.
  model_.reset();

  // Break references to |this| because the popup may not be
  // deallocated immediately.
  [matrix_ setObserver:NULL];
}

// Background colors for different states of the popup elements.
// static
NSColor* OmniboxPopupViewMac::BackgroundColor(bool is_dark_theme) {
  const CGFloat kMDDarkControlBackground = 40 / 255.;
  return is_dark_theme
             ? [NSColor colorWithGenericGamma22White:kMDDarkControlBackground
                                               alpha:1]
             : [NSColor controlBackgroundColor];
}

bool OmniboxPopupViewMac::IsOpen() const {
  return popup_ != nil;
}

void OmniboxPopupViewMac::UpdatePopupAppearance() {
  DCHECK([NSThread isMainThread]);
  model_->autocomplete_controller()->InlineTailPrefixes();
  const AutocompleteResult& result = GetResult();
  const size_t rows = result.size();
  if (rows == 0) {
    [[popup_ parentWindow] removeChildWindow:popup_];
    [popup_ orderOut:nil];

    // Break references to |this| because the popup may not be
    // deallocated immediately.
    [matrix_ setObserver:NULL];
    matrix_.reset();

    popup_.reset(nil);
    return;
  }

  CreatePopupIfNeeded();

  NSImage* answerImage = nil;
  const size_t result_size = model_->result().size();
  for (size_t i = 0; i < result_size; ++i) {
    const SkBitmap* bitmap = model_->RichSuggestionBitmapAt(i);
    if (result.match_at(i).answer && bitmap != nullptr) {
      answerImage = gfx::Image::CreateFrom1xBitmap(*bitmap).CopyNSImage();
      break;
    }
  }
  [matrix_ setController:[[[OmniboxPopupTableController alloc]
                             initWithMatchResults:result
                                        tableView:matrix_
                                        popupView:*this
                                      answerImage:answerImage] autorelease]];
  BOOL is_dark_theme = [matrix_ hasDarkTheme];
  [matrix_ setSeparator:[OmniboxPopupCell
                            createSeparatorStringForDarkTheme:is_dark_theme]];

  // Update the selection before placing (and displaying) the window.
  PaintUpdatesNow();

  // Calculate the matrix size manually rather than using -sizeToCells
  // because actually resizing the matrix messed up the popup size
  // animation.
  DCHECK_EQ([matrix_ intercellSpacing].height, 0.0);
  PositionPopup(NSHeight([matrix_ frame]));
}

void OmniboxPopupViewMac::OnMatchIconUpdated(size_t match_index) {
  [matrix_ setMatchIcon:ImageForMatch(GetResult().match_at(match_index))
                 forRow:match_index];
}

// This is only called by model in SetSelectedLine() after updating
// everything.  Popup should already be visible.
void OmniboxPopupViewMac::PaintUpdatesNow() {
  [matrix_ selectRowIndex:model_->selected_line()];
}

void OmniboxPopupViewMac::OnMatrixRowSelected(OmniboxPopupMatrix* matrix,
                                              size_t row) {
  model_->SetSelectedLine(row, false, false);
}

void OmniboxPopupViewMac::OnMatrixRowClicked(OmniboxPopupMatrix* matrix,
                                             size_t row) {
  OpenURLForRow(row,
                ui::WindowOpenDispositionFromNSEvent([NSApp currentEvent]));
}

void OmniboxPopupViewMac::OnMatrixRowMiddleClicked(OmniboxPopupMatrix* matrix,
                                                   size_t row) {
  OpenURLForRow(row, WindowOpenDisposition::NEW_BACKGROUND_TAB);
}

const AutocompleteResult& OmniboxPopupViewMac::GetResult() const {
  return model_->result();
}

void OmniboxPopupViewMac::CreatePopupIfNeeded() {
  if (!popup_) {
    popup_.reset(
        [[NSWindow alloc] initWithContentRect:ui::kWindowSizeDeterminedLater
                                    styleMask:NSBorderlessWindowMask
                                      backing:NSBackingStoreBuffered
                                        defer:NO]);
    [popup_ setBackgroundColor:[NSColor clearColor]];
    [popup_ setOpaque:NO];

    // Use a flipped view to pin the matrix top the top left. This is needed
    // for animated resize.
    base::scoped_nsobject<FlippedView> contentView(
        [[FlippedView alloc] initWithFrame:NSZeroRect]);
    [popup_ setContentView:contentView];

    BOOL is_dark_theme = [[field_ window] hasDarkTheme];

    // View to draw a background beneath the matrix.
    background_view_.reset([[NSBox alloc] initWithFrame:NSZeroRect]);
    [background_view_ setBoxType:NSBoxCustom];
    [background_view_ setBorderType:NSNoBorder];
    [background_view_ setFillColor:BackgroundColor(is_dark_theme)];
    [background_view_ setContentViewMargins:NSZeroSize];
    [contentView addSubview:background_view_];

    matrix_.reset([[OmniboxPopupMatrix alloc] initWithObserver:this
                                                  forDarkTheme:is_dark_theme]);
    [background_view_ addSubview:matrix_];

    top_separator_view_.reset(
        [[OmniboxPopupTopSeparatorView alloc] initWithFrame:NSZeroRect]);
    [contentView addSubview:top_separator_view_];

    bottom_separator_view_.reset([[OmniboxPopupBottomSeparatorView alloc]
        initWithFrame:NSZeroRect
         forDarkTheme:is_dark_theme]);
    [contentView addSubview:bottom_separator_view_];

    // TODO(dtseng): Ignore until we provide NSAccessibility support.
    [popup_ accessibilitySetOverrideValue:NSAccessibilityUnknownRole
                             forAttribute:NSAccessibilityRoleAttribute];
  }
}

void OmniboxPopupViewMac::PositionPopup(const CGFloat matrixHeight) {
  BrowserWindowController* controller =
      [BrowserWindowController browserWindowControllerForView:field_];
  NSRect anchor_rect_base = [controller omniboxPopupAnchorRect];

  // Calculate the popup's position on the screen.
  NSRect popup_frame = anchor_rect_base;

  CGFloat table_width = NSWidth([[[field_ window] contentView] bounds]);
  DCHECK_GT(table_width, 0.0);

  NSPoint field_origin_base =
      [field_ convertPoint:[field_ bounds].origin toView:nil];

  // Size to fit the matrix and shift down by the size.
  popup_frame.size.height = matrixHeight + PopupPaddingVertical() * 2.0;
  popup_frame.size.height += [OmniboxPopupTopSeparatorView preferredHeight];
  popup_frame.size.height += [OmniboxPopupBottomSeparatorView preferredHeight];
  popup_frame.origin.x = 0;
  popup_frame.origin.y -= NSHeight(popup_frame);

  // Shift to screen coordinates.
  if ([controller window]) {
    popup_frame = [[controller window] convertRectToScreen:popup_frame];
  }

  // Top separator.
  NSRect top_separator_frame = NSZeroRect;
  top_separator_frame.size.width = NSWidth(popup_frame);
  top_separator_frame.size.height =
      [OmniboxPopupTopSeparatorView preferredHeight];
  [top_separator_view_ setFrame:top_separator_frame];

  // Bottom separator.
  NSRect bottom_separator_frame = NSZeroRect;
  bottom_separator_frame.size.width = NSWidth(popup_frame);
  bottom_separator_frame.size.height =
      [OmniboxPopupBottomSeparatorView preferredHeight];
  bottom_separator_frame.origin.y =
      NSHeight(popup_frame) - NSHeight(bottom_separator_frame);
  [bottom_separator_view_ setFrame:bottom_separator_frame];

  // Background view.
  NSRect background_rect = NSZeroRect;
  background_rect.size.width = NSWidth(popup_frame);
  background_rect.size.height = NSHeight(popup_frame) -
      NSHeight(top_separator_frame) - NSHeight(bottom_separator_frame);
  background_rect.origin.y = NSMaxY(top_separator_frame);
  [background_view_ setFrame:background_rect];

  // Matrix.
  NSRect matrix_frame = NSZeroRect;
  matrix_frame.origin.x = 0;
  [matrix_ setContentLeftPadding:field_origin_base.x];
  [matrix_ setContentMaxWidth:NSWidth([field_ bounds])];
  matrix_frame.origin.y = PopupPaddingVertical();
  matrix_frame.size.width = table_width;
  matrix_frame.size.height = matrixHeight;
  [matrix_ setFrame:matrix_frame];
  [[[matrix_ tableColumns] objectAtIndex:0] setWidth:table_width];

  // Don't play animation games on first display.
  if (![popup_ parentWindow]) {
    DCHECK(![popup_ isVisible]);
    [popup_ setFrame:popup_frame display:NO];
    [[field_ window] addChildWindow:popup_ ordered:NSWindowAbove];
    return;
  }
  DCHECK([popup_ isVisible]);

  // Animate the frame change if the only change is that the height got smaller.
  // Otherwise, resize immediately.
  NSRect current_popup_frame = [popup_ frame];
  bool animate = (NSHeight(popup_frame) < NSHeight(current_popup_frame) &&
                  NSWidth(popup_frame) == NSWidth(current_popup_frame));

  base::scoped_nsobject<NSDictionary> savedAnimations;
  if (!animate) {
    // In an ideal world, running a zero-length animation would cancel any
    // running animations and set the new frame value immediately.  In practice,
    // zero-length animations are ignored entirely.  Work around this AppKit bug
    // by explicitly setting an NSNull animation for the "frame" key and then
    // running the animation with a non-zero(!!) duration.  This somehow
    // convinces AppKit to do the right thing.  Save off the current animations
    // dictionary so it can be restored later.
    savedAnimations.reset([[popup_ animations] copy]);
    [popup_ setAnimations:@{@"frame" : [NSNull null]}];
  }

  [NSAnimationContext beginGrouping];
  // Don't use the GTM addition for the "Steve" slowdown because this can
  // happen async from user actions and the effects could be a surprise.
  [[NSAnimationContext currentContext] setDuration:kShrinkAnimationDuration];
  // When using the animator to update |popup_| on El Capitan, for some reason
  // the window does not get redrawn. Use a completion handler to make sure
  // |popup_| gets redrawn once the animation completes. See
  // http://crbug.com/538590 and http://crbug.com/551007 .
  if (base::mac::IsAtLeastOS10_11()) {
    NSWindow* popup = popup_.get();
    [[NSAnimationContext currentContext] setCompletionHandler:^{
      [popup display];
    }];
  }
  [[popup_ animator] setFrame:popup_frame display:YES];
  [NSAnimationContext endGrouping];

  if (!animate) {
    // Restore the original animations dictionary.  This does not reinstate any
    // previously running animations.
    [popup_ setAnimations:savedAnimations];
  }
}

NSImage* OmniboxPopupViewMac::ImageForMatch(
    const AutocompleteMatch& match) const {
  bool is_dark_mode = [matrix_ hasDarkTheme];
  const SkColor vector_icon_color =
      is_dark_mode ? SkColorSetA(SK_ColorWHITE, 0xCC) : gfx::kChromeIconGrey;
  return model_->GetMatchIcon(match, vector_icon_color).ToNSImage();
}

void OmniboxPopupViewMac::OpenURLForRow(size_t row,
                                        WindowOpenDisposition disposition) {
  DCHECK_LT(row, GetResult().size());
  omnibox_view_->OpenMatch(GetResult().match_at(row), disposition, GURL(),
                           base::string16(), row);
}
