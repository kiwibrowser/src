/*
 * This file is part of the theme implementation for form controls in WebCore.
 *
 * Copyright (C) 2005 Apple Computer, Inc.
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
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_THEME_MAC_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_THEME_MAC_H_

#import <AppKit/AppKit.h>

#import "third_party/blink/renderer/core/layout/layout_theme.h"
#import "third_party/blink/renderer/core/paint/theme_painter_mac.h"
#import "third_party/blink/renderer/platform/wtf/hash_map.h"
#import "third_party/blink/renderer/platform/wtf/retain_ptr.h"

OBJC_CLASS BlinkLayoutThemeNotificationObserver;

namespace blink {

class LayoutThemeMac final : public LayoutTheme {
 public:
  static scoped_refptr<LayoutTheme> Create();

  void AddVisualOverflow(const Node*,
                         const ComputedStyle&,
                         IntRect& border_box) override;

  bool IsControlStyled(const ComputedStyle&) const override;

  Color PlatformActiveSelectionBackgroundColor() const override;
  Color PlatformInactiveSelectionBackgroundColor() const override;
  Color PlatformActiveSelectionForegroundColor() const override;
  Color PlatformActiveListBoxSelectionBackgroundColor() const override;
  Color PlatformActiveListBoxSelectionForegroundColor() const override;
  Color PlatformInactiveListBoxSelectionBackgroundColor() const override;
  Color PlatformInactiveListBoxSelectionForegroundColor() const override;
  Color PlatformSpellingMarkerUnderlineColor() const override;
  Color PlatformGrammarMarkerUnderlineColor() const override;
  Color PlatformFocusRingColor() const override;

  ScrollbarControlSize ScrollbarControlSizeForPart(ControlPart part) override {
    return part == kListboxPart ? kSmallScrollbar : kRegularScrollbar;
  }

  void PlatformColorsDidChange() override;

  // System fonts.
  void SystemFont(CSSValueID system_font_id,
                  FontSelectionValue& font_slope,
                  FontSelectionValue& font_weight,
                  float& font_size,
                  AtomicString& font_family) const override;

  int MinimumMenuListSize(const ComputedStyle&) const override;

  void AdjustSliderThumbSize(ComputedStyle&) const override;

  IntSize SliderTickSize() const override;
  int SliderTickOffsetFromTrackCenter() const override;

  int PopupInternalPaddingStart(const ComputedStyle&) const override;
  int PopupInternalPaddingEnd(const ChromeClient*,
                              const ComputedStyle&) const override;
  int PopupInternalPaddingTop(const ComputedStyle&) const override;
  int PopupInternalPaddingBottom(const ComputedStyle&) const override;

  bool PopsMenuByArrowKeys() const override { return true; }
  bool PopsMenuBySpaceKey() const final { return true; }

  // Returns the repeat interval of the animation for the progress bar.
  double AnimationRepeatIntervalForProgressBar() const override;
  // Returns the duration of the animation for the progress bar.
  double AnimationDurationForProgressBar() const override;

  Color SystemColor(CSSValueID) const override;

  bool SupportsSelectionForegroundColors() const override { return false; }

  bool IsModalColorChooser() const override { return false; }

 protected:
  LayoutThemeMac();
  ~LayoutThemeMac() override;

  void AdjustMenuListStyle(ComputedStyle&, Element*) const override;
  void AdjustMenuListButtonStyle(ComputedStyle&, Element*) const override;
  void AdjustSearchFieldStyle(ComputedStyle&) const override;
  void AdjustSearchFieldCancelButtonStyle(ComputedStyle&) const override;

 public:
  // Constants and methods shared with ThemePainterMac

  // Get the control size based off the font. Used by some of the controls (like
  // buttons).
  NSControlSize ControlSizeForFont(const ComputedStyle&) const;
  NSControlSize ControlSizeForSystemFont(const ComputedStyle&) const;
  void SetControlSize(NSCell*,
                      const IntSize* sizes,
                      const IntSize& min_size,
                      float zoom_level = 1.0f);
  void SetSizeFromFont(ComputedStyle&, const IntSize* sizes) const;
  IntSize SizeForFont(const ComputedStyle&, const IntSize* sizes) const;
  IntSize SizeForSystemFont(const ComputedStyle&, const IntSize* sizes) const;
  void SetFontFromControlSize(ComputedStyle&, NSControlSize) const;

  void UpdateCheckedState(NSCell*, const Node*);
  void UpdateEnabledState(NSCell*, const Node*);
  void UpdateFocusedState(NSCell*, const Node*, const ComputedStyle&);
  void UpdatePressedState(NSCell*, const Node*);

  // Helpers for adjusting appearance and for painting

  void SetPopupButtonCellState(const Node*,
                               const ComputedStyle&,
                               const IntRect&);
  const IntSize* PopupButtonSizes() const;
  const int* PopupButtonMargins() const;
  const int* PopupButtonPadding(NSControlSize) const;
  const IntSize* MenuListSizes() const;

  const IntSize* SearchFieldSizes() const;
  const IntSize* CancelButtonSizes() const;
  void SetSearchCellState(const Node*, const ComputedStyle&, const IntRect&);
  void SetSearchFieldSize(ComputedStyle&) const;

  NSPopUpButtonCell* PopupButton() const;
  NSSearchFieldCell* Search() const;
  NSTextFieldCell* TextField() const;

  // A view associated to the contained document. Subclasses may not have such a
  // view and return a fake.
  NSView* DocumentView() const;

  void UpdateActiveState(NSCell*, const Node*);

  // We estimate the animation rate of a Mac OS X progress bar is 33 fps.
  // Hard code the value here because we haven't found API for it.
  static constexpr double kProgressAnimationFrameRate = 0.033;
  // Mac OS X progress bar animation seems to have 256 frames.
  static constexpr double kProgressAnimationNumFrames = 256;

  static constexpr float kBaseFontSize = 11.0f;
  static constexpr float kMenuListBaseArrowHeight = 4.0f;
  static constexpr float kMenuListBaseArrowWidth = 5.0f;
  static constexpr float kMenuListBaseSpaceBetweenArrows = 2.0f;
  static const int kMenuListArrowPaddingStart = 4;
  static const int kMenuListArrowPaddingEnd = 4;
  static const int kSliderThumbWidth = 15;
  static const int kSliderThumbHeight = 15;
  static const int kSliderThumbShadowBlur = 1;
  static const int kSliderThumbBorderWidth = 1;
  static const int kSliderTrackWidth = 5;
  static const int kSliderTrackBorderWidth = 1;

 protected:
  String ExtraFullscreenStyleSheet() override;

  // Controls color values returned from platformFocusRingColor(). systemColor()
  // will be used when false.
  bool UsesTestModeFocusRingColor() const;

  bool ShouldUseFallbackTheme(const ComputedStyle&) const override;

  void AdjustProgressBarBounds(ComputedStyle&) const override;

 private:
  const int* ProgressBarHeights() const;
  const int* ProgressBarMargins(NSControlSize) const;
  String FileListNameForWidth(Locale&,
                              const FileList*,
                              const Font&,
                              int width) const override;
  String ExtraDefaultStyleSheet() override;
  bool ThemeDrawsFocusRing(const ComputedStyle&) const override;

  ThemePainter& Painter() override { return painter_; }

  mutable RetainPtr<NSPopUpButtonCell> popup_button_;
  mutable RetainPtr<NSSearchFieldCell> search_;
  mutable RetainPtr<NSMenu> search_menu_template_;
  mutable RetainPtr<NSLevelIndicatorCell> level_indicator_;
  mutable RetainPtr<NSTextFieldCell> text_field_;

  mutable HashMap<int, RGBA32> system_color_cache_;

  RetainPtr<BlinkLayoutThemeNotificationObserver> notification_observer_;

  ThemePainterMac painter_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_THEME_MAC_H_
