/*
 * Copyright (C) 2008, 2010, 2011, 2012 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "third_party/blink/renderer/platform/mac/theme_mac.h"

#import <Carbon/Carbon.h>
#import "third_party/blink/renderer/platform/graphics/graphics_context_state_saver.h"
#import "third_party/blink/renderer/platform/mac/block_exceptions.h"
#import "third_party/blink/renderer/platform/mac/local_current_graphics_context.h"
#import "third_party/blink/renderer/platform/mac/version_util_mac.h"
#import "third_party/blink/renderer/platform/mac/web_core_ns_cell_extras.h"
#import "third_party/blink/renderer/platform/scroll/scrollable_area.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"

// This is a view whose sole purpose is to tell AppKit that it's flipped.
@interface BlinkFlippedControl : NSControl
@end

@implementation BlinkFlippedControl

- (BOOL)isFlipped {
  return YES;
}

- (NSText*)currentEditor {
  return nil;
}

- (BOOL)_automaticFocusRingDisabled {
  return YES;
}

@end

namespace blink {

Theme* PlatformTheme() {
  DEFINE_STATIC_LOCAL(ThemeMac, theme_mac, ());
  return &theme_mac;
}

// Helper functions used by a bunch of different control parts.

static NSControlSize ControlSizeForFont(
    const FontDescription& font_description) {
  int font_size = font_description.ComputedPixelSize();
  if (font_size >= 16)
    return NSRegularControlSize;
  if (font_size >= 11)
    return NSSmallControlSize;
  return NSMiniControlSize;
}

static LengthSize SizeFromNSControlSize(NSControlSize ns_control_size,
                                        const LengthSize& zoomed_size,
                                        float zoom_factor,
                                        const IntSize* sizes) {
  IntSize control_size = sizes[ns_control_size];
  if (zoom_factor != 1.0f)
    control_size = IntSize(control_size.Width() * zoom_factor,
                           control_size.Height() * zoom_factor);
  LengthSize result = zoomed_size;
  if (zoomed_size.Width().IsIntrinsicOrAuto() && control_size.Width() > 0)
    result.SetWidth(Length(control_size.Width(), kFixed));
  if (zoomed_size.Height().IsIntrinsicOrAuto() && control_size.Height() > 0)
    result.SetHeight(Length(control_size.Height(), kFixed));
  return result;
}

static LengthSize SizeFromFont(const FontDescription& font_description,
                               const LengthSize& zoomed_size,
                               float zoom_factor,
                               const IntSize* sizes) {
  return SizeFromNSControlSize(ControlSizeForFont(font_description),
                               zoomed_size, zoom_factor, sizes);
}

NSControlSize ThemeMac::ControlSizeFromPixelSize(const IntSize* sizes,
                                                 const IntSize& min_zoomed_size,
                                                 float zoom_factor) {
  if (min_zoomed_size.Width() >=
          static_cast<int>(sizes[NSRegularControlSize].Width() * zoom_factor) &&
      min_zoomed_size.Height() >=
          static_cast<int>(sizes[NSRegularControlSize].Height() * zoom_factor))
    return NSRegularControlSize;
  if (min_zoomed_size.Width() >=
          static_cast<int>(sizes[NSSmallControlSize].Width() * zoom_factor) &&
      min_zoomed_size.Height() >=
          static_cast<int>(sizes[NSSmallControlSize].Height() * zoom_factor))
    return NSSmallControlSize;
  return NSMiniControlSize;
}

static void SetControlSize(NSCell* cell,
                           const IntSize* sizes,
                           const IntSize& min_zoomed_size,
                           float zoom_factor) {
  ControlSize size =
      ThemeMac::ControlSizeFromPixelSize(sizes, min_zoomed_size, zoom_factor);
  // Only update if we have to, since AppKit does work even if the size is the
  // same.
  if (size != [cell controlSize])
    [cell setControlSize:(NSControlSize)size];
}

static void UpdateStates(NSCell* cell, ControlStates states) {
  // Hover state is not supported by Aqua.

  // Pressed state
  bool old_pressed = [cell isHighlighted];
  bool pressed = states & kPressedControlState;
  if (pressed != old_pressed)
    [cell setHighlighted:pressed];

  // Enabled state
  bool old_enabled = [cell isEnabled];
  bool enabled = states & kEnabledControlState;
  if (enabled != old_enabled)
    [cell setEnabled:enabled];

  // Checked and Indeterminate
  bool old_indeterminate = [cell state] == NSMixedState;
  bool indeterminate = (states & kIndeterminateControlState);
  bool checked = states & kCheckedControlState;
  bool old_checked = [cell state] == NSOnState;
  if (old_indeterminate != indeterminate || checked != old_checked)
    [cell setState:indeterminate ? NSMixedState
                                 : (checked ? NSOnState : NSOffState)];

  // Window inactive state does not need to be checked explicitly, since we
  // paint parented to a view in a window whose key state can be detected.
}

// Return a fake NSView whose sole purpose is to tell AppKit that it's flipped.
NSView* ThemeMac::EnsuredView(ScrollableArea* scrollable_area) {
  // Use a fake flipped view.
  static NSView* flipped_view = [[BlinkFlippedControl alloc] init];
  [flipped_view
      setFrameSize:NSSizeFromCGSize(CGSize(scrollable_area->ContentsSize()))];

  return flipped_view;
}

// static
IntRect ThemeMac::InflateRect(const IntRect& zoomed_rect,
                              const IntSize& zoomed_size,
                              const int* margins,
                              float zoom_factor) {
  // Only do the inflation if the available width/height are too small.
  // Otherwise try to fit the glow/check space into the available box's
  // width/height.
  int width_delta = zoomed_rect.Width() -
                    (zoomed_size.Width() + margins[kLeftMargin] * zoom_factor +
                     margins[kRightMargin] * zoom_factor);
  int height_delta = zoomed_rect.Height() -
                     (zoomed_size.Height() + margins[kTopMargin] * zoom_factor +
                      margins[kBottomMargin] * zoom_factor);
  IntRect result(zoomed_rect);
  if (width_delta < 0) {
    result.SetX(result.X() - margins[kLeftMargin] * zoom_factor);
    result.SetWidth(result.Width() - width_delta);
  }
  if (height_delta < 0) {
    result.SetY(result.Y() - margins[kTopMargin] * zoom_factor);
    result.SetHeight(result.Height() - height_delta);
  }
  return result;
}

// static
IntRect ThemeMac::InflateRectForAA(const IntRect& rect) {
  const int kMargin = 2;
  return IntRect(rect.X() - kMargin, rect.Y() - kMargin,
                 rect.Width() + 2 * kMargin, rect.Height() + 2 * kMargin);
}

// static
IntRect ThemeMac::InflateRectForFocusRing(const IntRect& rect) {
  // Just put a margin of 16 units around the rect. The UI elements that use
  // this don't appropriately scale their focus rings appropriately (e.g, paint
  // pickers), or switch to non-native widgets when scaled (e.g, check boxes
  // and radio buttons).
  const int kMargin = 16;
  IntRect result;
  result.SetX(rect.X() - kMargin);
  result.SetY(rect.Y() - kMargin);
  result.SetWidth(rect.Width() + 2 * kMargin);
  result.SetHeight(rect.Height() + 2 * kMargin);
  return result;
}

// Checkboxes

const IntSize* ThemeMac::CheckboxSizes() {
  static const IntSize kSizes[3] = {IntSize(14, 14), IntSize(12, 12),
                                    IntSize(10, 10)};
  return kSizes;
}

const int* ThemeMac::CheckboxMargins(NSControlSize control_size) {
  static const int kMargins[3][4] = {
      {3, 4, 4, 2}, {4, 3, 3, 3}, {4, 3, 3, 3},
  };
  return kMargins[control_size];
}

LengthSize ThemeMac::CheckboxSize(const FontDescription& font_description,
                                  const LengthSize& zoomed_size,
                                  float zoom_factor) {
  // If the width and height are both specified, then we have nothing to do.
  if (!zoomed_size.Width().IsIntrinsicOrAuto() &&
      !zoomed_size.Height().IsIntrinsicOrAuto())
    return zoomed_size;

  // Use the font size to determine the intrinsic width of the control.
  return SizeFromFont(font_description, zoomed_size, zoom_factor,
                      CheckboxSizes());
}

NSButtonCell* ThemeMac::Checkbox(ControlStates states,
                                 const IntRect& zoomed_rect,
                                 float zoom_factor) {
  static NSButtonCell* checkbox_cell;
  if (!checkbox_cell) {
    checkbox_cell = [[NSButtonCell alloc] init];
    [checkbox_cell setButtonType:NSSwitchButton];
    [checkbox_cell setTitle:nil];
    [checkbox_cell setAllowsMixedState:YES];
    [checkbox_cell setFocusRingType:NSFocusRingTypeExterior];
  }

  // Set the control size based off the rectangle we're painting into.
  SetControlSize(checkbox_cell, CheckboxSizes(), zoomed_rect.Size(),
                 zoom_factor);

  // Update the various states we respond to.
  UpdateStates(checkbox_cell, states);

  return checkbox_cell;
}

const IntSize* ThemeMac::RadioSizes() {
  static const IntSize kSizes[3] = {IntSize(14, 15), IntSize(12, 13),
                                    IntSize(10, 10)};
  return kSizes;
}

const int* ThemeMac::RadioMargins(NSControlSize control_size) {
  static const int kMargins[3][4] = {
      {2, 2, 4, 2}, {3, 2, 3, 2}, {1, 0, 2, 0},
  };
  return kMargins[control_size];
}

LengthSize ThemeMac::RadioSize(const FontDescription& font_description,
                               const LengthSize& zoomed_size,
                               float zoom_factor) {
  // If the width and height are both specified, then we have nothing to do.
  if (!zoomed_size.Width().IsIntrinsicOrAuto() &&
      !zoomed_size.Height().IsIntrinsicOrAuto())
    return zoomed_size;

  // Use the font size to determine the intrinsic width of the control.
  return SizeFromFont(font_description, zoomed_size, zoom_factor, RadioSizes());
}

NSButtonCell* ThemeMac::Radio(ControlStates states,
                              const IntRect& zoomed_rect,
                              float zoom_factor) {
  static NSButtonCell* radio_cell;
  if (!radio_cell) {
    radio_cell = [[NSButtonCell alloc] init];
    [radio_cell setButtonType:NSRadioButton];
    [radio_cell setTitle:nil];
    [radio_cell setFocusRingType:NSFocusRingTypeExterior];
  }

  // Set the control size based off the rectangle we're painting into.
  SetControlSize(radio_cell, RadioSizes(), zoomed_rect.Size(), zoom_factor);

  // Update the various states we respond to.
  // Cocoa draws NSMixedState NSRadioButton as NSOnState so we don't want that.
  states &= ~kIndeterminateControlState;
  UpdateStates(radio_cell, states);

  return radio_cell;
}

// Buttons really only constrain height. They respect width.
const IntSize* ThemeMac::ButtonSizes() {
  static const IntSize kSizes[3] = {IntSize(0, 21), IntSize(0, 18),
                                    IntSize(0, 15)};
  return kSizes;
}

const int* ThemeMac::ButtonMargins(NSControlSize control_size) {
  static const int kMargins[3][4] = {
      {4, 6, 7, 6}, {4, 5, 6, 5}, {0, 1, 1, 1},
  };
  return kMargins[control_size];
}

static void SetUpButtonCell(NSButtonCell* cell,
                            ControlPart part,
                            ControlStates states,
                            const IntRect& zoomed_rect,
                            float zoom_factor) {
  // Set the control size based off the rectangle we're painting into.
  const IntSize* sizes = ThemeMac::ButtonSizes();
  if (part == kSquareButtonPart ||
      zoomed_rect.Height() >
          ThemeMac::ButtonSizes()[NSRegularControlSize].Height() *
              zoom_factor) {
    // Use the square button
    if ([cell bezelStyle] != NSShadowlessSquareBezelStyle)
      [cell setBezelStyle:NSShadowlessSquareBezelStyle];
  } else if ([cell bezelStyle] != NSRoundedBezelStyle)
    [cell setBezelStyle:NSRoundedBezelStyle];

  SetControlSize(cell, sizes, zoomed_rect.Size(), zoom_factor);

  // Update the various states we respond to.
  UpdateStates(cell, states);
}

NSButtonCell* ThemeMac::Button(ControlPart part,
                               ControlStates states,
                               const IntRect& zoomed_rect,
                               float zoom_factor) {
  static NSButtonCell* cell = nil;
  if (!cell) {
    cell = [[NSButtonCell alloc] init];
    [cell setTitle:nil];
    [cell setButtonType:NSMomentaryPushInButton];
  }
  SetUpButtonCell(cell, part, states, zoomed_rect, zoom_factor);
  return cell;
}

const IntSize* ThemeMac::StepperSizes() {
  static const IntSize kSizes[3] = {IntSize(19, 27), IntSize(15, 22),
                                    IntSize(13, 15)};
  return kSizes;
}

// We don't use controlSizeForFont() for steppers because the stepper height
// should be equal to or less than the corresponding text field height,
static NSControlSize StepperControlSizeForFont(
    const FontDescription& font_description) {
  int font_size = font_description.ComputedPixelSize();
  if (font_size >= 27)
    return NSRegularControlSize;
  if (font_size >= 22)
    return NSSmallControlSize;
  return NSMiniControlSize;
}

// Theme overrides

int ThemeMac::BaselinePositionAdjustment(ControlPart part) const {
  if (part == kCheckboxPart || part == kRadioPart)
    return -2;
  return Theme::BaselinePositionAdjustment(part);
}

FontDescription ThemeMac::ControlFont(ControlPart part,
                                      const FontDescription& font_description,
                                      float zoom_factor) const {
  switch (part) {
    case kPushButtonPart: {
      FontDescription result;
      result.SetIsAbsoluteSize(true);
      result.SetGenericFamily(FontDescription::kSerifFamily);

      NSFont* ns_font = [NSFont
          systemFontOfSize:[NSFont systemFontSizeForControlSize:
                                       ControlSizeForFont(font_description)]];
      result.FirstFamily().SetFamily(FontFamilyNames::system_ui);
      result.SetComputedSize([ns_font pointSize] * zoom_factor);
      result.SetSpecifiedSize([ns_font pointSize] * zoom_factor);
      return result;
    }
    default:
      return Theme::ControlFont(part, font_description, zoom_factor);
  }
}

LengthSize ThemeMac::GetControlSize(ControlPart part,
                                    const FontDescription& font_description,
                                    const LengthSize& zoomed_size,
                                    float zoom_factor) const {
  switch (part) {
    case kCheckboxPart:
      return CheckboxSize(font_description, zoomed_size, zoom_factor);
    case kRadioPart:
      return RadioSize(font_description, zoomed_size, zoom_factor);
    case kPushButtonPart:
      // Height is reset to auto so that specified heights can be ignored.
      return SizeFromFont(font_description,
                          LengthSize(zoomed_size.Width(), Length()),
                          zoom_factor, ButtonSizes());
    case kInnerSpinButtonPart:
      if (!zoomed_size.Width().IsIntrinsicOrAuto() &&
          !zoomed_size.Height().IsIntrinsicOrAuto())
        return zoomed_size;
      return SizeFromNSControlSize(StepperControlSizeForFont(font_description),
                                   zoomed_size, zoom_factor, StepperSizes());
    default:
      return zoomed_size;
  }
}

LengthSize ThemeMac::MinimumControlSize(ControlPart part,
                                        const FontDescription& font_description,
                                        float zoom_factor) const {
  switch (part) {
    case kSquareButtonPart:
    case kButtonPart:
      return LengthSize(Length(0, kFixed),
                        Length(static_cast<int>(15 * zoom_factor), kFixed));
    case kInnerSpinButtonPart: {
      IntSize base = StepperSizes()[NSMiniControlSize];
      return LengthSize(
          Length(static_cast<int>(base.Width() * zoom_factor), kFixed),
          Length(static_cast<int>(base.Height() * zoom_factor), kFixed));
    }
    default:
      return Theme::MinimumControlSize(part, font_description, zoom_factor);
  }
}

LengthBox ThemeMac::ControlBorder(ControlPart part,
                                  const FontDescription& font_description,
                                  const LengthBox& zoomed_box,
                                  float zoom_factor) const {
  switch (part) {
    case kSquareButtonPart:
      return LengthBox(0, zoomed_box.Right().Value(), 0,
                       zoomed_box.Left().Value());
    default:
      return Theme::ControlBorder(part, font_description, zoomed_box,
                                  zoom_factor);
  }
}

LengthBox ThemeMac::ControlPadding(ControlPart part,
                                   const FontDescription& font_description,
                                   const Length& zoomed_box_top,
                                   const Length& zoomed_box_right,
                                   const Length& zoomed_box_bottom,
                                   const Length& zoomed_box_left,
                                   float zoom_factor) const {
  switch (part) {
    case kPushButtonPart: {
      // Just use 8px.  AppKit wants to use 11px for mini buttons, but that
      // padding is just too large for real-world Web sites (creating a huge
      // necessary minimum width for buttons whose space is by definition
      // constrained, since we select mini only for small cramped environments.
      // This also guarantees the HTML <button> will match our rendering by
      // default, since we're using a consistent padding.
      const int padding = 8 * zoom_factor;
      return LengthBox(2, padding, 3, padding);
    }
    default:
      return Theme::ControlPadding(part, font_description, zoomed_box_top,
                                   zoomed_box_right, zoomed_box_bottom,
                                   zoomed_box_left, zoom_factor);
  }
}

void ThemeMac::AddVisualOverflow(ControlPart part,
                                 ControlStates states,
                                 float zoom_factor,
                                 IntRect& zoomed_rect) const {
  BEGIN_BLOCK_OBJC_EXCEPTIONS
  switch (part) {
    case kCheckboxPart: {
      // We inflate the rect as needed to account for padding included in the
      // cell to accommodate the checkbox shadow" and the check.  We don't
      // consider this part of the bounds of the control in WebKit.
      NSCell* cell = Checkbox(states, zoomed_rect, zoom_factor);
      NSControlSize control_size = [cell controlSize];
      IntSize zoomed_size = CheckboxSizes()[control_size];
      zoomed_size.SetHeight(zoomed_size.Height() * zoom_factor);
      zoomed_size.SetWidth(zoomed_size.Width() * zoom_factor);
      zoomed_rect = InflateRect(zoomed_rect, zoomed_size,
                                CheckboxMargins(control_size), zoom_factor);
      break;
    }
    case kRadioPart: {
      // We inflate the rect as needed to account for padding included in the
      // cell to accommodate the radio button shadow".  We don't consider this
      // part of the bounds of the control in WebKit.
      NSCell* cell = Radio(states, zoomed_rect, zoom_factor);
      NSControlSize control_size = [cell controlSize];
      IntSize zoomed_size = RadioSizes()[control_size];
      zoomed_size.SetHeight(zoomed_size.Height() * zoom_factor);
      zoomed_size.SetWidth(zoomed_size.Width() * zoom_factor);
      zoomed_rect = InflateRect(zoomed_rect, zoomed_size,
                                RadioMargins(control_size), zoom_factor);
      break;
    }
    case kPushButtonPart:
    case kButtonPart: {
      NSButtonCell* cell = Button(part, states, zoomed_rect, zoom_factor);
      NSControlSize control_size = [cell controlSize];

      // We inflate the rect as needed to account for the Aqua button's shadow.
      if ([cell bezelStyle] == NSRoundedBezelStyle) {
        IntSize zoomed_size = ButtonSizes()[control_size];
        zoomed_size.SetHeight(zoomed_size.Height() * zoom_factor);
        // Buttons don't ever constrain width, so the zoomed width can just be
        // honored.
        zoomed_size.SetWidth(zoomed_rect.Width());
        zoomed_rect = InflateRect(zoomed_rect, zoomed_size,
                                  ButtonMargins(control_size), zoom_factor);
      }
      break;
    }
    case kInnerSpinButtonPart: {
      static const int kStepperMargin[4] = {0, 0, 0, 0};
      ControlSize control_size = ControlSizeFromPixelSize(
          StepperSizes(), zoomed_rect.Size(), zoom_factor);
      IntSize zoomed_size = StepperSizes()[control_size];
      zoomed_size.SetHeight(zoomed_size.Height() * zoom_factor);
      zoomed_size.SetWidth(zoomed_size.Width() * zoom_factor);
      zoomed_rect =
          InflateRect(zoomed_rect, zoomed_size, kStepperMargin, zoom_factor);
      break;
    }
    default:
      break;
  }
  END_BLOCK_OBJC_EXCEPTIONS
}
}
