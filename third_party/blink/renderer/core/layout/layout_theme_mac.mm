/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Google, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#import "third_party/blink/renderer/core/layout/layout_theme_mac.h"

#import <AvailabilityMacros.h>
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <math.h>
#import "third_party/blink/renderer/core/css_value_keywords.h"
#import "third_party/blink/renderer/core/fileapi/file_list.h"
#import "third_party/blink/renderer/core/html_names.h"
#import "third_party/blink/renderer/core/layout/layout_progress.h"
#import "third_party/blink/renderer/core/layout/layout_view.h"
#import "third_party/blink/renderer/core/style/shadow_list.h"
#import "third_party/blink/renderer/platform/data_resource_helper.h"
#import "third_party/blink/renderer/platform/fonts/string_truncator.h"
#import "third_party/blink/renderer/platform/graphics/bitmap_image.h"
#import "third_party/blink/renderer/platform/layout_test_support.h"
#import "third_party/blink/renderer/platform/mac/color_mac.h"
#import "third_party/blink/renderer/platform/mac/theme_mac.h"
#import "third_party/blink/renderer/platform/mac/version_util_mac.h"
#import "third_party/blink/renderer/platform/mac/web_core_ns_cell_extras.h"
#import "third_party/blink/renderer/platform/runtime_enabled_features.h"
#import "third_party/blink/renderer/platform/text/platform_locale.h"
#import "third_party/blink/renderer/platform/theme.h"

// The methods in this file are specific to the Mac OS X platform.

@interface BlinkLayoutThemeNotificationObserver : NSObject {
  blink::LayoutTheme* _theme;
}

- (id)initWithTheme:(blink::LayoutTheme*)theme;
- (void)systemColorsDidChange:(NSNotification*)notification;

@end

@implementation BlinkLayoutThemeNotificationObserver

- (id)initWithTheme:(blink::LayoutTheme*)theme {
  if (!(self = [super init]))
    return nil;

  _theme = theme;
  return self;
}

- (void)systemColorsDidChange:(NSNotification*)unusedNotification {
  DCHECK([[unusedNotification name]
      isEqualToString:NSSystemColorsDidChangeNotification]);
  _theme->PlatformColorsDidChange();
}

@end

@interface NSTextFieldCell (WKDetails)
- (CFDictionaryRef)_coreUIDrawOptionsWithFrame:(NSRect)cellFrame
                                        inView:(NSView*)controlView
                                  includeFocus:(BOOL)includeFocus;
@end

@interface BlinkTextFieldCell : NSTextFieldCell
- (CFDictionaryRef)_coreUIDrawOptionsWithFrame:(NSRect)cellFrame
                                        inView:(NSView*)controlView
                                  includeFocus:(BOOL)includeFocus;
@end

@implementation BlinkTextFieldCell
- (CFDictionaryRef)_coreUIDrawOptionsWithFrame:(NSRect)cellFrame
                                        inView:(NSView*)controlView
                                  includeFocus:(BOOL)includeFocus {
  // FIXME: This is a post-Lion-only workaround for <rdar://problem/11385461>.
  // When that bug is resolved, we should remove this code.
  CFMutableDictionaryRef coreUIDrawOptions = CFDictionaryCreateMutableCopy(
      NULL, 0, [super _coreUIDrawOptionsWithFrame:cellFrame
                                           inView:controlView
                                     includeFocus:includeFocus]);
  CFDictionarySetValue(coreUIDrawOptions, @"borders only", kCFBooleanTrue);
  return (CFDictionaryRef)[NSMakeCollectable(coreUIDrawOptions) autorelease];
}
@end

@interface BlinkFlippedView : NSView
@end

@implementation BlinkFlippedView

- (BOOL)isFlipped {
  return YES;
}

- (NSText*)currentEditor {
  return nil;
}

@end

namespace blink {

namespace {

bool FontSizeMatchesToControlSize(const ComputedStyle& style) {
  int font_size = style.FontSize();
  if (font_size == [NSFont systemFontSizeForControlSize:NSRegularControlSize])
    return true;
  if (font_size == [NSFont systemFontSizeForControlSize:NSSmallControlSize])
    return true;
  if (font_size == [NSFont systemFontSizeForControlSize:NSMiniControlSize])
    return true;
  return false;
}

NSColor* ColorInColorSpace(NSColor* color) {
  return [color colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
}

}  // namespace

using namespace HTMLNames;

LayoutThemeMac::LayoutThemeMac()
    : LayoutTheme(PlatformTheme()),
      notification_observer_(
          kAdoptNS,
          [[BlinkLayoutThemeNotificationObserver alloc] initWithTheme:this]),
      painter_(*this) {
  [[NSNotificationCenter defaultCenter]
      addObserver:notification_observer_.Get()
         selector:@selector(systemColorsDidChange:)
             name:NSSystemColorsDidChangeNotification
           object:nil];
}

LayoutThemeMac::~LayoutThemeMac() {
  [[NSNotificationCenter defaultCenter]
      removeObserver:notification_observer_.Get()];
}

Color LayoutThemeMac::PlatformActiveSelectionBackgroundColor() const {
  NSColor* color = ColorInColorSpace([NSColor selectedTextBackgroundColor]);
  return Color(static_cast<int>(255.0 * [color redComponent]),
               static_cast<int>(255.0 * [color greenComponent]),
               static_cast<int>(255.0 * [color blueComponent]));
}

Color LayoutThemeMac::PlatformInactiveSelectionBackgroundColor() const {
  NSColor* color = ColorInColorSpace([NSColor secondarySelectedControlColor]);
  return Color(static_cast<int>(255.0 * [color redComponent]),
               static_cast<int>(255.0 * [color greenComponent]),
               static_cast<int>(255.0 * [color blueComponent]));
}

Color LayoutThemeMac::PlatformActiveSelectionForegroundColor() const {
  return Color::kBlack;
}

Color LayoutThemeMac::PlatformActiveListBoxSelectionBackgroundColor() const {
  NSColor* color = ColorInColorSpace([NSColor alternateSelectedControlColor]);
  return Color(static_cast<int>(255.0 * [color redComponent]),
               static_cast<int>(255.0 * [color greenComponent]),
               static_cast<int>(255.0 * [color blueComponent]));
}

Color LayoutThemeMac::PlatformActiveListBoxSelectionForegroundColor() const {
  return Color::kWhite;
}

Color LayoutThemeMac::PlatformInactiveListBoxSelectionForegroundColor() const {
  return Color::kBlack;
}

Color LayoutThemeMac::PlatformSpellingMarkerUnderlineColor() const {
  return Color(251, 45, 29);
}

Color LayoutThemeMac::PlatformGrammarMarkerUnderlineColor() const {
  return Color(107, 107, 107);
}

Color LayoutThemeMac::PlatformFocusRingColor() const {
  static const RGBA32 kOldAquaFocusRingColor = 0xFF7DADD9;
  if (UsesTestModeFocusRingColor())
    return kOldAquaFocusRingColor;

  return SystemColor(CSSValueWebkitFocusRingColor);
}

Color LayoutThemeMac::PlatformInactiveListBoxSelectionBackgroundColor() const {
  return PlatformInactiveSelectionBackgroundColor();
}

static FontSelectionValue ToFontWeight(NSInteger app_kit_font_weight) {
  DCHECK_GT(app_kit_font_weight, 0);
  DCHECK_LT(app_kit_font_weight, 15);
  if (app_kit_font_weight > 14)
    app_kit_font_weight = 14;
  else if (app_kit_font_weight < 1)
    app_kit_font_weight = 1;

  static FontSelectionValue font_weights[] = {
      FontSelectionValue(100), FontSelectionValue(100), FontSelectionValue(200),
      FontSelectionValue(300), FontSelectionValue(400), FontSelectionValue(500),
      FontSelectionValue(600), FontSelectionValue(600), FontSelectionValue(700),
      FontSelectionValue(800), FontSelectionValue(800), FontSelectionValue(900),
      FontSelectionValue(900), FontSelectionValue(900)};
  return font_weights[app_kit_font_weight - 1];
}

static inline NSFont* SystemNSFont(CSSValueID system_font_id) {
  switch (system_font_id) {
    case CSSValueSmallCaption:
      return [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
    case CSSValueMenu:
      return [NSFont menuFontOfSize:[NSFont systemFontSize]];
    case CSSValueStatusBar:
      return [NSFont labelFontOfSize:[NSFont labelFontSize]];
    case CSSValueWebkitMiniControl:
      return [NSFont
          systemFontOfSize:[NSFont
                               systemFontSizeForControlSize:NSMiniControlSize]];
    case CSSValueWebkitSmallControl:
      return [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:
                                                  NSSmallControlSize]];
    case CSSValueWebkitControl:
      return [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:
                                                  NSRegularControlSize]];
    default:
      return [NSFont systemFontOfSize:[NSFont systemFontSize]];
  }
}

void LayoutThemeMac::SystemFont(CSSValueID system_font_id,
                                FontSelectionValue& font_slope,
                                FontSelectionValue& font_weight,
                                float& font_size,
                                AtomicString& font_family) const {
  NSFont* font = SystemNSFont(system_font_id);
  if (!font)
    return;

  NSFontManager* font_manager = [NSFontManager sharedFontManager];
  font_slope = ([font_manager traitsOfFont:font] & NSItalicFontMask)
                   ? ItalicSlopeValue()
                   : NormalSlopeValue();
  font_weight = ToFontWeight([font_manager weightOfFont:font]);
  font_size = [font pointSize];
  font_family = FontFamilyNames::system_ui;
}

static RGBA32 ConvertNSColorToColor(NSColor* color) {
  NSColor* color_in_color_space = ColorInColorSpace(color);
  if (color_in_color_space) {
    static const double kScaleFactor = nextafter(256.0, 0.0);
    return MakeRGB(
        static_cast<int>(kScaleFactor * [color_in_color_space redComponent]),
        static_cast<int>(kScaleFactor * [color_in_color_space greenComponent]),
        static_cast<int>(kScaleFactor * [color_in_color_space blueComponent]));
  }

  // This conversion above can fail if the NSColor in question is an
  // NSPatternColor (as many system colors are). These colors are actually a
  // repeating pattern not just a solid color. To work around this we simply
  // draw a 1x1 image of the color and use that pixel's color. It might be
  // better to use an average of the colors in the pattern instead.
  NSBitmapImageRep* offscreen_rep =
      [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nil
                                              pixelsWide:1
                                              pixelsHigh:1
                                           bitsPerSample:8
                                         samplesPerPixel:4
                                                hasAlpha:YES
                                                isPlanar:NO
                                          colorSpaceName:NSDeviceRGBColorSpace
                                             bytesPerRow:4
                                            bitsPerPixel:32];

  [NSGraphicsContext saveGraphicsState];
  [NSGraphicsContext
      setCurrentContext:[NSGraphicsContext
                            graphicsContextWithBitmapImageRep:offscreen_rep]];
  NSEraseRect(NSMakeRect(0, 0, 1, 1));
  [color drawSwatchInRect:NSMakeRect(0, 0, 1, 1)];
  [NSGraphicsContext restoreGraphicsState];

  NSUInteger pixel[4];
  [offscreen_rep getPixel:pixel atX:0 y:0];
  [offscreen_rep release];
  // This recursive call will not recurse again, because the color space
  // the second time around is NSDeviceRGBColorSpace.
  return ConvertNSColorToColor([NSColor colorWithDeviceRed:pixel[0] / 255.
                                                     green:pixel[1] / 255.
                                                      blue:pixel[2] / 255.
                                                     alpha:1.]);
}

static RGBA32 MenuBackgroundColor() {
  NSBitmapImageRep* offscreen_rep =
      [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nil
                                              pixelsWide:1
                                              pixelsHigh:1
                                           bitsPerSample:8
                                         samplesPerPixel:4
                                                hasAlpha:YES
                                                isPlanar:NO
                                          colorSpaceName:NSDeviceRGBColorSpace
                                             bytesPerRow:4
                                            bitsPerPixel:32];

  CGContextRef context = static_cast<CGContextRef>([[NSGraphicsContext
      graphicsContextWithBitmapImageRep:offscreen_rep] graphicsPort]);
  CGRect rect = CGRectMake(0, 0, 1, 1);
  HIThemeMenuDrawInfo draw_info;
  draw_info.version = 0;
  draw_info.menuType = kThemeMenuTypePopUp;
  HIThemeDrawMenuBackground(&rect, &draw_info, context,
                            kHIThemeOrientationInverted);

  NSUInteger pixel[4];
  [offscreen_rep getPixel:pixel atX:0 y:0];
  [offscreen_rep release];
  return ConvertNSColorToColor([NSColor colorWithDeviceRed:pixel[0] / 255.
                                                     green:pixel[1] / 255.
                                                      blue:pixel[2] / 255.
                                                     alpha:1.]);
}

void LayoutThemeMac::PlatformColorsDidChange() {
  system_color_cache_.clear();
  LayoutTheme::PlatformColorsDidChange();
}

Color LayoutThemeMac::SystemColor(CSSValueID css_value_id) const {
  {
    HashMap<int, RGBA32>::iterator it = system_color_cache_.find(css_value_id);
    if (it != system_color_cache_.end())
      return it->value;
  }

  Color color;
  bool needs_fallback = false;
  switch (css_value_id) {
    case CSSValueActiveborder:
      color = ConvertNSColorToColor([NSColor keyboardFocusIndicatorColor]);
      break;
    case CSSValueActivecaption:
      color = ConvertNSColorToColor([NSColor windowFrameTextColor]);
      break;
    case CSSValueAppworkspace:
      color = ConvertNSColorToColor([NSColor headerColor]);
      break;
    case CSSValueBackground:
      // Use theme independent default
      needs_fallback = true;
      break;
    case CSSValueButtonface:
      color = ConvertNSColorToColor([NSColor controlBackgroundColor]);
      break;
    case CSSValueButtonhighlight:
      color = ConvertNSColorToColor([NSColor controlHighlightColor]);
      break;
    case CSSValueButtonshadow:
      color = ConvertNSColorToColor([NSColor controlShadowColor]);
      break;
    case CSSValueButtontext:
      color = ConvertNSColorToColor([NSColor controlTextColor]);
      break;
    case CSSValueCaptiontext:
      color = ConvertNSColorToColor([NSColor textColor]);
      break;
    case CSSValueGraytext:
      color = ConvertNSColorToColor([NSColor disabledControlTextColor]);
      break;
    case CSSValueHighlight:
      color = ConvertNSColorToColor([NSColor selectedTextBackgroundColor]);
      break;
    case CSSValueHighlighttext:
      color = ConvertNSColorToColor([NSColor selectedTextColor]);
      break;
    case CSSValueInactiveborder:
      color = ConvertNSColorToColor([NSColor controlBackgroundColor]);
      break;
    case CSSValueInactivecaption:
      color = ConvertNSColorToColor([NSColor controlBackgroundColor]);
      break;
    case CSSValueInactivecaptiontext:
      color = ConvertNSColorToColor([NSColor textColor]);
      break;
    case CSSValueInfobackground:
      // There is no corresponding NSColor for this so we use a hard coded
      // value.
      color = 0xFFFBFCC5;
      break;
    case CSSValueInfotext:
      color = ConvertNSColorToColor([NSColor textColor]);
      break;
    case CSSValueMenu:
      color = MenuBackgroundColor();
      break;
    case CSSValueMenutext:
      color = ConvertNSColorToColor([NSColor selectedMenuItemTextColor]);
      break;
    case CSSValueScrollbar:
      color = ConvertNSColorToColor([NSColor scrollBarColor]);
      break;
    case CSSValueText:
      color = ConvertNSColorToColor([NSColor textColor]);
      break;
    case CSSValueThreeddarkshadow:
      color = ConvertNSColorToColor([NSColor controlDarkShadowColor]);
      break;
    case CSSValueThreedshadow:
      color = ConvertNSColorToColor([NSColor shadowColor]);
      break;
    case CSSValueThreedface:
      // We use this value instead of NSColor's controlColor to avoid website
      // incompatibilities. We may want to change this to use the NSColor in
      // future.
      color = 0xFFC0C0C0;
      break;
    case CSSValueThreedhighlight:
      color = ConvertNSColorToColor([NSColor highlightColor]);
      break;
    case CSSValueThreedlightshadow:
      color = ConvertNSColorToColor([NSColor controlLightHighlightColor]);
      break;
    case CSSValueWebkitFocusRingColor:
      color = ConvertNSColorToColor([NSColor keyboardFocusIndicatorColor]);
      break;
    case CSSValueWindow:
      color = ConvertNSColorToColor([NSColor windowBackgroundColor]);
      break;
    case CSSValueWindowframe:
      color = ConvertNSColorToColor([NSColor windowFrameColor]);
      break;
    case CSSValueWindowtext:
      color = ConvertNSColorToColor([NSColor windowFrameTextColor]);
      break;
    default:
      needs_fallback = true;
      break;
  }

  if (needs_fallback)
    color = LayoutTheme::SystemColor(css_value_id);

  system_color_cache_.Set(css_value_id, color.Rgb());

  return color;
}

bool LayoutThemeMac::IsControlStyled(const ComputedStyle& style) const {
  if (style.Appearance() == kTextFieldPart ||
      style.Appearance() == kTextAreaPart)
    return style.HasAuthorBorder() || style.BoxShadow();

  if (style.Appearance() == kMenulistPart) {
    // FIXME: This is horrible, but there is not much else that can be done.
    // Menu lists cannot draw properly when scaled. They can't really draw
    // properly when transformed either. We can't detect the transform case
    // at style adjustment time so that will just have to stay broken.  We
    // can however detect that we're zooming. If zooming is in effect we
    // treat it like the control is styled.
    if (style.EffectiveZoom() != 1.0f)
      return true;
    if (!FontSizeMatchesToControlSize(style))
      return true;
    if (style.GetFontDescription().Family().Family() !=
        FontFamilyNames::system_ui)
      return true;
    if (!style.Height().IsIntrinsicOrAuto())
      return true;
  }
  // Some other cells don't work well when scaled.
  if (style.EffectiveZoom() != 1) {
    switch (style.Appearance()) {
      case kButtonPart:
      case kPushButtonPart:
      case kSearchFieldPart:
      case kSquareButtonPart:
        return true;
      default:
        break;
    }
  }
  return LayoutTheme::IsControlStyled(style);
}

void LayoutThemeMac::AddVisualOverflow(const Node* node,
                                       const ComputedStyle& style,
                                       IntRect& rect) {
  ControlPart part = style.Appearance();

  if (HasPlatformTheme()) {
    switch (part) {
      case kCheckboxPart:
      case kRadioPart:
      case kPushButtonPart:
      case kSquareButtonPart:
      case kButtonPart:
      case kInnerSpinButtonPart:
        return LayoutTheme::AddVisualOverflow(node, style, rect);
      default:
        break;
    }
  }

  float zoom_level = style.EffectiveZoom();

  if (part == kMenulistPart) {
    SetPopupButtonCellState(node, style, rect);
    IntSize size = PopupButtonSizes()[[PopupButton() controlSize]];
    size.SetHeight(size.Height() * zoom_level);
    size.SetWidth(rect.Width());
    rect = ThemeMac::InflateRect(rect, size, PopupButtonMargins(), zoom_level);
  } else if (part == kSliderThumbHorizontalPart ||
             part == kSliderThumbVerticalPart) {
    rect.SetHeight(rect.Height() + kSliderThumbShadowBlur);
  }
}

void LayoutThemeMac::UpdateCheckedState(NSCell* cell, const Node* node) {
  bool old_indeterminate = [cell state] == NSMixedState;
  bool indeterminate = IsIndeterminate(node);
  bool checked = IsChecked(node);

  if (old_indeterminate != indeterminate) {
    [cell setState:indeterminate ? NSMixedState
                                 : (checked ? NSOnState : NSOffState)];
    return;
  }

  bool old_checked = [cell state] == NSOnState;
  if (checked != old_checked)
    [cell setState:checked ? NSOnState : NSOffState];
}

void LayoutThemeMac::UpdateEnabledState(NSCell* cell, const Node* node) {
  bool old_enabled = [cell isEnabled];
  bool enabled = IsEnabled(node);
  if (enabled != old_enabled)
    [cell setEnabled:enabled];
}

void LayoutThemeMac::UpdateFocusedState(NSCell* cell,
                                        const Node* node,
                                        const ComputedStyle& style) {
  bool old_focused = [cell showsFirstResponder];
  bool focused = IsFocused(node) && style.OutlineStyleIsAuto();
  if (focused != old_focused)
    [cell setShowsFirstResponder:focused];
}

void LayoutThemeMac::UpdatePressedState(NSCell* cell, const Node* node) {
  bool old_pressed = [cell isHighlighted];
  bool pressed = node && node->IsActive();
  if (pressed != old_pressed)
    [cell setHighlighted:pressed];
}

NSControlSize LayoutThemeMac::ControlSizeForFont(
    const ComputedStyle& style) const {
  int font_size = style.FontSize();
  if (font_size >= 16)
    return NSRegularControlSize;
  if (font_size >= 11)
    return NSSmallControlSize;
  return NSMiniControlSize;
}

void LayoutThemeMac::SetControlSize(NSCell* cell,
                                    const IntSize* sizes,
                                    const IntSize& min_size,
                                    float zoom_level) {
  NSControlSize size;
  if (min_size.Width() >=
          static_cast<int>(sizes[NSRegularControlSize].Width() * zoom_level) &&
      min_size.Height() >=
          static_cast<int>(sizes[NSRegularControlSize].Height() * zoom_level))
    size = NSRegularControlSize;
  else if (min_size.Width() >=
               static_cast<int>(sizes[NSSmallControlSize].Width() *
                                zoom_level) &&
           min_size.Height() >=
               static_cast<int>(sizes[NSSmallControlSize].Height() *
                                zoom_level))
    size = NSSmallControlSize;
  else
    size = NSMiniControlSize;
  // Only update if we have to, since AppKit does work even if the size is the
  // same.
  if (size != [cell controlSize])
    [cell setControlSize:size];
}

IntSize LayoutThemeMac::SizeForFont(const ComputedStyle& style,
                                    const IntSize* sizes) const {
  if (style.EffectiveZoom() != 1.0f) {
    IntSize result = sizes[ControlSizeForFont(style)];
    return IntSize(result.Width() * style.EffectiveZoom(),
                   result.Height() * style.EffectiveZoom());
  }
  return sizes[ControlSizeForFont(style)];
}

IntSize LayoutThemeMac::SizeForSystemFont(const ComputedStyle& style,
                                          const IntSize* sizes) const {
  if (style.EffectiveZoom() != 1.0f) {
    IntSize result = sizes[ControlSizeForSystemFont(style)];
    return IntSize(result.Width() * style.EffectiveZoom(),
                   result.Height() * style.EffectiveZoom());
  }
  return sizes[ControlSizeForSystemFont(style)];
}

void LayoutThemeMac::SetSizeFromFont(ComputedStyle& style,
                                     const IntSize* sizes) const {
  // FIXME: Check is flawed, since it doesn't take min-width/max-width into
  // account.
  IntSize size = SizeForFont(style, sizes);
  if (style.Width().IsIntrinsicOrAuto() && size.Width() > 0)
    style.SetWidth(Length(size.Width(), kFixed));
  if (style.Height().IsAuto() && size.Height() > 0)
    style.SetHeight(Length(size.Height(), kFixed));
}

void LayoutThemeMac::SetFontFromControlSize(ComputedStyle& style,
                                            NSControlSize control_size) const {
  FontDescription font_description;
  font_description.SetIsAbsoluteSize(true);
  font_description.SetGenericFamily(FontDescription::kSerifFamily);

  NSFont* font = [NSFont
      systemFontOfSize:[NSFont systemFontSizeForControlSize:control_size]];
  font_description.FirstFamily().SetFamily(FontFamilyNames::system_ui);
  font_description.SetComputedSize([font pointSize] * style.EffectiveZoom());
  font_description.SetSpecifiedSize([font pointSize] * style.EffectiveZoom());

  // Reset line height.
  style.SetLineHeight(ComputedStyleInitialValues::InitialLineHeight());

  // TODO(esprehn): The fontSelector manual management is buggy and error prone.
  FontSelector* font_selector = style.GetFont().GetFontSelector();
  if (style.SetFontDescription(font_description))
    style.GetFont().Update(font_selector);
}

NSControlSize LayoutThemeMac::ControlSizeForSystemFont(
    const ComputedStyle& style) const {
  float font_size = style.FontSize();
  float zoom_level = style.EffectiveZoom();
  if (zoom_level != 1)
    font_size /= zoom_level;
  if (font_size >= [NSFont systemFontSizeForControlSize:NSRegularControlSize])
    return NSRegularControlSize;
  if (font_size >= [NSFont systemFontSizeForControlSize:NSSmallControlSize])
    return NSSmallControlSize;
  return NSMiniControlSize;
}

const int* LayoutThemeMac::PopupButtonMargins() const {
  static const int kMargins[3][4] = {{0, 3, 1, 3}, {0, 3, 2, 3}, {0, 1, 0, 1}};
  return kMargins[[PopupButton() controlSize]];
}

const IntSize* LayoutThemeMac::PopupButtonSizes() const {
  static const IntSize kSizes[3] = {IntSize(0, 21), IntSize(0, 18),
                                    IntSize(0, 15)};
  return kSizes;
}

const int* LayoutThemeMac::PopupButtonPadding(NSControlSize size) const {
  static const int kPadding[3][4] = {
      {2, 26, 3, 8}, {2, 23, 3, 8}, {2, 22, 3, 10}};
  return kPadding[size];
}

const int* LayoutThemeMac::ProgressBarHeights() const {
  static const int kSizes[3] = {20, 12, 12};
  return kSizes;
}

double LayoutThemeMac::AnimationRepeatIntervalForProgressBar() const {
  return kProgressAnimationFrameRate;
}

double LayoutThemeMac::AnimationDurationForProgressBar() const {
  return kProgressAnimationNumFrames * kProgressAnimationFrameRate;
}

static const IntSize* MenuListButtonSizes() {
  static const IntSize kSizes[3] = {IntSize(0, 21), IntSize(0, 18),
                                    IntSize(0, 15)};
  return kSizes;
}

void LayoutThemeMac::AdjustMenuListStyle(ComputedStyle& style,
                                         Element* e) const {
  NSControlSize control_size = ControlSizeForFont(style);

  style.ResetBorder();
  style.ResetPadding();

  // Height is locked to auto.
  style.SetHeight(Length(kAuto));

  // White-space is locked to pre.
  style.SetWhiteSpace(EWhiteSpace::kPre);

  // Set the foreground color to black or gray when we have the aqua look.
  // Cast to RGB32 is to work around a compiler bug.
  style.SetColor(e && !e->IsDisabledFormControl()
                     ? static_cast<RGBA32>(Color::kBlack)
                     : Color::kDarkGray);

  // Set the button's vertical size.
  SetSizeFromFont(style, MenuListButtonSizes());

  // Our font is locked to the appropriate system font size for the
  // control. To clarify, we first use the CSS-specified font to figure out a
  // reasonable control size, but once that control size is determined, we
  // throw that font away and use the appropriate system font for the control
  // size instead.
  SetFontFromControlSize(style, control_size);
}

static const int kBaseBorderRadius = 5;
static const int kStyledPopupPaddingStart = 8;
static const int kStyledPopupPaddingTop = 1;
static const int kStyledPopupPaddingBottom = 2;

// These functions are called with MenuListPart or MenulistButtonPart appearance
// by LayoutMenuList.
int LayoutThemeMac::PopupInternalPaddingStart(
    const ComputedStyle& style) const {
  if (style.Appearance() == kMenulistPart)
    return PopupButtonPadding(
               ControlSizeForFont(style))[ThemeMac::kLeftMargin] *
           style.EffectiveZoom();
  if (style.Appearance() == kMenulistButtonPart)
    return kStyledPopupPaddingStart * style.EffectiveZoom();
  return 0;
}

int LayoutThemeMac::PopupInternalPaddingEnd(const ChromeClient*,
                                            const ComputedStyle& style) const {
  if (style.Appearance() == kMenulistPart)
    return PopupButtonPadding(
               ControlSizeForFont(style))[ThemeMac::kRightMargin] *
           style.EffectiveZoom();
  if (style.Appearance() != kMenulistButtonPart)
    return 0;
  float font_scale = style.FontSize() / kBaseFontSize;
  float arrow_width = kMenuListBaseArrowWidth * font_scale;
  return static_cast<int>(ceilf(
      arrow_width + (kMenuListArrowPaddingStart + kMenuListArrowPaddingEnd) *
                        style.EffectiveZoom()));
}

int LayoutThemeMac::PopupInternalPaddingTop(const ComputedStyle& style) const {
  if (style.Appearance() == kMenulistPart)
    return PopupButtonPadding(ControlSizeForFont(style))[ThemeMac::kTopMargin] *
           style.EffectiveZoom();
  if (style.Appearance() == kMenulistButtonPart)
    return kStyledPopupPaddingTop * style.EffectiveZoom();
  return 0;
}

int LayoutThemeMac::PopupInternalPaddingBottom(
    const ComputedStyle& style) const {
  if (style.Appearance() == kMenulistPart)
    return PopupButtonPadding(
               ControlSizeForFont(style))[ThemeMac::kBottomMargin] *
           style.EffectiveZoom();
  if (style.Appearance() == kMenulistButtonPart)
    return kStyledPopupPaddingBottom * style.EffectiveZoom();
  return 0;
}

void LayoutThemeMac::AdjustMenuListButtonStyle(ComputedStyle& style,
                                               Element*) const {
  float font_scale = style.FontSize() / kBaseFontSize;

  style.ResetPadding();
  style.SetBorderRadius(
      IntSize(int(kBaseBorderRadius + font_scale - 1),
              int(kBaseBorderRadius + font_scale - 1)));  // FIXME: Round up?

  const int kMinHeight = 15;
  style.SetMinHeight(Length(kMinHeight, kFixed));

  style.SetLineHeight(ComputedStyleInitialValues::InitialLineHeight());
}

void LayoutThemeMac::SetPopupButtonCellState(const Node* node,
                                             const ComputedStyle& style,
                                             const IntRect& rect) {
  NSPopUpButtonCell* popup_button = this->PopupButton();

  // Set the control size based off the rectangle we're painting into.
  SetControlSize(popup_button, PopupButtonSizes(), rect.Size(),
                 style.EffectiveZoom());

  // Update the various states we respond to.
  UpdateActiveState(popup_button, node);
  UpdateCheckedState(popup_button, node);
  UpdateEnabledState(popup_button, node);
  UpdatePressedState(popup_button, node);

  popup_button.userInterfaceLayoutDirection =
      style.Direction() == TextDirection::kLtr
          ? NSUserInterfaceLayoutDirectionLeftToRight
          : NSUserInterfaceLayoutDirectionRightToLeft;
}

const IntSize* LayoutThemeMac::MenuListSizes() const {
  static const IntSize kSizes[3] = {IntSize(9, 0), IntSize(5, 0),
                                    IntSize(0, 0)};
  return kSizes;
}

int LayoutThemeMac::MinimumMenuListSize(const ComputedStyle& style) const {
  return SizeForSystemFont(style, MenuListSizes()).Width();
}

void LayoutThemeMac::SetSearchCellState(const Node* node,
                                        const ComputedStyle& style,
                                        const IntRect&) {
  NSSearchFieldCell* search = this->Search();

  // Update the various states we respond to.
  UpdateActiveState(search, node);
  UpdateEnabledState(search, node);
  UpdateFocusedState(search, node, style);
}

const IntSize* LayoutThemeMac::SearchFieldSizes() const {
  static const IntSize kSizes[3] = {IntSize(0, 22), IntSize(0, 19),
                                    IntSize(0, 15)};
  return kSizes;
}

static const int* SearchFieldHorizontalPaddings() {
  static const int kSizes[3] = {3, 2, 1};
  return kSizes;
}

void LayoutThemeMac::SetSearchFieldSize(ComputedStyle& style) const {
  // If the width and height are both specified, then we have nothing to do.
  if (!style.Width().IsIntrinsicOrAuto() && !style.Height().IsAuto())
    return;

  // Use the font size to determine the intrinsic width of the control.
  SetSizeFromFont(style, SearchFieldSizes());
}

const int kSearchFieldBorderWidth = 2;
void LayoutThemeMac::AdjustSearchFieldStyle(ComputedStyle& style) const {
  // Override border.
  style.ResetBorder();
  const short border_width = kSearchFieldBorderWidth * style.EffectiveZoom();
  style.SetBorderLeftWidth(border_width);
  style.SetBorderLeftStyle(EBorderStyle::kInset);
  style.SetBorderRightWidth(border_width);
  style.SetBorderRightStyle(EBorderStyle::kInset);
  style.SetBorderBottomWidth(border_width);
  style.SetBorderBottomStyle(EBorderStyle::kInset);
  style.SetBorderTopWidth(border_width);
  style.SetBorderTopStyle(EBorderStyle::kInset);

  // Override height.
  style.SetHeight(Length(kAuto));
  SetSearchFieldSize(style);

  NSControlSize control_size = ControlSizeForFont(style);

  // Override padding size to match AppKit text positioning.
  const int vertical_padding = 1 * style.EffectiveZoom();
  const int horizontal_padding =
      SearchFieldHorizontalPaddings()[control_size] * style.EffectiveZoom();
  style.SetPaddingLeft(Length(horizontal_padding, kFixed));
  style.SetPaddingRight(Length(horizontal_padding, kFixed));
  style.SetPaddingTop(Length(vertical_padding, kFixed));
  style.SetPaddingBottom(Length(vertical_padding, kFixed));

  SetFontFromControlSize(style, control_size);

  style.SetBoxShadow(nullptr);
}

const IntSize* LayoutThemeMac::CancelButtonSizes() const {
  static const IntSize kSizes[3] = {IntSize(14, 14), IntSize(11, 11),
                                    IntSize(9, 9)};
  return kSizes;
}

void LayoutThemeMac::AdjustSearchFieldCancelButtonStyle(
    ComputedStyle& style) const {
  IntSize size = SizeForSystemFont(style, CancelButtonSizes());
  style.SetWidth(Length(size.Width(), kFixed));
  style.SetHeight(Length(size.Height(), kFixed));
  style.SetBoxShadow(nullptr);
}

IntSize LayoutThemeMac::SliderTickSize() const {
  return IntSize(1, 3);
}

int LayoutThemeMac::SliderTickOffsetFromTrackCenter() const {
  return -9;
}

void LayoutThemeMac::AdjustProgressBarBounds(ComputedStyle& style) const {
  float zoom_level = style.EffectiveZoom();
  NSControlSize control_size = ControlSizeForFont(style);
  int height = ProgressBarHeights()[control_size] * zoom_level;

  // Now inflate it to account for the shadow.
  style.SetMinHeight(Length(height + zoom_level, kFixed));
}

void LayoutThemeMac::AdjustSliderThumbSize(ComputedStyle& style) const {
  float zoom_level = style.EffectiveZoom();
  if (style.Appearance() == kSliderThumbHorizontalPart ||
      style.Appearance() == kSliderThumbVerticalPart) {
    style.SetWidth(
        Length(static_cast<int>(kSliderThumbWidth * zoom_level), kFixed));
    style.SetHeight(
        Length(static_cast<int>(kSliderThumbHeight * zoom_level), kFixed));
  }
}

NSPopUpButtonCell* LayoutThemeMac::PopupButton() const {
  if (!popup_button_) {
    popup_button_.AdoptNS(
        [[NSPopUpButtonCell alloc] initTextCell:@"" pullsDown:NO]);
    [popup_button_.Get() setUsesItemFromMenu:NO];
    [popup_button_.Get() setFocusRingType:NSFocusRingTypeExterior];
  }

  return popup_button_.Get();
}

NSSearchFieldCell* LayoutThemeMac::Search() const {
  if (!search_) {
    search_.AdoptNS([[NSSearchFieldCell alloc] initTextCell:@""]);
    [search_.Get() setBezelStyle:NSTextFieldRoundedBezel];
    [search_.Get() setBezeled:YES];
    [search_.Get() setEditable:YES];
    [search_.Get() setFocusRingType:NSFocusRingTypeExterior];

    // Suppress NSSearchFieldCell's default placeholder text. Prior to OS10.11,
    // this is achieved by calling |setCenteredLook| with NO. In OS10.11 and
    // later, instead call |setPlaceholderString| with an empty string.
    // See https://crbug.com/752362.
    if (IsOS10_10()) {
      SEL sel = @selector(setCenteredLook:);
      if ([search_.Get() respondsToSelector:sel]) {
        BOOL bool_value = NO;
        NSMethodSignature* signature =
            [NSSearchFieldCell instanceMethodSignatureForSelector:sel];
        NSInvocation* invocation =
            [NSInvocation invocationWithMethodSignature:signature];
        [invocation setTarget:search_.Get()];
        [invocation setSelector:sel];
        [invocation setArgument:&bool_value atIndex:2];
        [invocation invoke];
      }
    } else {
      [search_.Get() setPlaceholderString:@""];
    }
  }

  return search_.Get();
}

NSTextFieldCell* LayoutThemeMac::TextField() const {
  if (!text_field_) {
    text_field_.AdoptNS([[BlinkTextFieldCell alloc] initTextCell:@""]);
    [text_field_.Get() setBezeled:YES];
    [text_field_.Get() setEditable:YES];
    [text_field_.Get() setFocusRingType:NSFocusRingTypeExterior];
    [text_field_.Get() setDrawsBackground:YES];
    [text_field_.Get() setBackgroundColor:[NSColor whiteColor]];
  }

  return text_field_.Get();
}

String LayoutThemeMac::FileListNameForWidth(Locale& locale,
                                            const FileList* file_list,
                                            const Font& font,
                                            int width) const {
  if (width <= 0)
    return String();

  String str_to_truncate;
  if (file_list->IsEmpty()) {
    str_to_truncate =
        locale.QueryString(WebLocalizedString::kFileButtonNoFileSelectedLabel);
  } else if (file_list->length() == 1) {
    File* file = file_list->item(0);
    if (file->GetUserVisibility() == File::kIsUserVisible)
      str_to_truncate = [[NSFileManager defaultManager]
          displayNameAtPath:(file_list->item(0)->GetPath())];
    else
      str_to_truncate = file->name();
  } else {
    return StringTruncator::RightTruncate(
        locale.QueryString(WebLocalizedString::kMultipleFileUploadText,
                           locale.ConvertToLocalizedNumber(
                               String::Number(file_list->length()))),
        width, font);
  }

  return StringTruncator::CenterTruncate(str_to_truncate, width, font);
}

NSView* FlippedView() {
  static NSView* view = [[BlinkFlippedView alloc] init];
  return view;
}

LayoutTheme& LayoutTheme::NativeTheme() {
  DEFINE_STATIC_REF(LayoutTheme, layout_theme, (LayoutThemeMac::Create()));
  return *layout_theme;
}

scoped_refptr<LayoutTheme> LayoutThemeMac::Create() {
  return base::AdoptRef(new LayoutThemeMac);
}

bool LayoutThemeMac::UsesTestModeFocusRingColor() const {
  return LayoutTestSupport::IsRunningLayoutTest();
}

NSView* LayoutThemeMac::DocumentView() const {
  return FlippedView();
}

// Updates the control tint (a.k.a. active state) of |cell| (from |o|).  In the
// Chromium port, the layoutObject runs as a background process and controls'
// NSCell(s) lack a parent NSView. Therefore controls don't have their tint
// color updated correctly when the application is activated/deactivated.
// FocusController's setActive() is called when the application is
// activated/deactivated, which causes a paint invalidation at which time this
// code is called.
// This function should be called before drawing any NSCell-derived controls,
// unless you're sure it isn't needed.
void LayoutThemeMac::UpdateActiveState(NSCell* cell, const Node* node) {
  NSControlTint old_tint = [cell controlTint];
  NSControlTint tint = IsActive(node)
                           ? [NSColor currentControlTint]
                           : static_cast<NSControlTint>(NSClearControlTint);

  if (tint != old_tint)
    [cell setControlTint:tint];
}

String LayoutThemeMac::ExtraFullscreenStyleSheet() {
  // FIXME: Chromium may wish to style its default media controls differently in
  // fullscreen.
  return String();
}

String LayoutThemeMac::ExtraDefaultStyleSheet() {
  return LayoutTheme::ExtraDefaultStyleSheet() +
         GetDataResourceAsASCIIString("themeInputMultipleFields.css") +
         GetDataResourceAsASCIIString("themeMac.css");
}

bool LayoutThemeMac::ThemeDrawsFocusRing(const ComputedStyle& style) const {
  if (ShouldUseFallbackTheme(style))
    return false;
  switch (style.Appearance()) {
    case kCheckboxPart:
    case kRadioPart:
    case kPushButtonPart:
    case kSquareButtonPart:
    case kButtonPart:
    case kMenulistPart:
    case kSliderThumbHorizontalPart:
    case kSliderThumbVerticalPart:
      return true;

    // Actually, they don't support native focus rings, but this function
    // returns true for them in order to prevent Blink from drawing focus rings.
    // SliderThumb*Part have focus rings, and we don't need to draw two focus
    // rings for single slider.
    case kSliderHorizontalPart:
    case kSliderVerticalPart:
      return true;

    default:
      return false;
  }
}

bool LayoutThemeMac::ShouldUseFallbackTheme(const ComputedStyle& style) const {
  ControlPart part = style.Appearance();
  if (part == kCheckboxPart || part == kRadioPart)
    return style.EffectiveZoom() != 1;
  return false;
}

}  // namespace blink
