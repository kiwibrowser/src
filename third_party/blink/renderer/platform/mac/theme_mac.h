/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_MAC_THEME_MAC_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_MAC_THEME_MAC_H_

#import <AppKit/AppKit.h>

#include "third_party/blink/renderer/platform/theme.h"

namespace blink {

class ThemeMac : public Theme {
 public:
  ThemeMac() {}
  ~ThemeMac() override {}

  int BaselinePositionAdjustment(ControlPart) const override;

  FontDescription ControlFont(ControlPart,
                              const FontDescription&,
                              float zoom_factor) const override;

  LengthSize GetControlSize(ControlPart,
                            const FontDescription&,
                            const LengthSize&,
                            float zoom_factor) const override;
  LengthSize MinimumControlSize(ControlPart,
                                const FontDescription&,
                                float zoom_factor) const override;

  LengthBox ControlPadding(ControlPart,
                           const FontDescription&,
                           const Length& zoomed_box_top,
                           const Length& zoomed_box_right,
                           const Length& zoomed_box_bottom,
                           const Length& zoomed_box_left,
                           float zoom_factor) const override;
  LengthBox ControlBorder(ControlPart,
                          const FontDescription&,
                          const LengthBox& zoomed_box,
                          float zoom_factor) const override;

  bool ControlRequiresPreWhiteSpace(ControlPart part) const override {
    return part == kPushButtonPart;
  }

  void AddVisualOverflow(ControlPart,
                         ControlStates,
                         float zoom_factor,
                         IntRect& border_box) const override;

  // Inflate an IntRect to accout for specific padding around margins.
  enum { kTopMargin = 0, kRightMargin = 1, kBottomMargin = 2, kLeftMargin = 3 };
  static PLATFORM_EXPORT IntRect InflateRect(const IntRect&,
                                             const IntSize&,
                                             const int* margins,
                                             float zoom_level = 1.0f);

  // Inflate an IntRect to account for any bleeding that would happen due to
  // anti-aliasing.
  static PLATFORM_EXPORT IntRect InflateRectForAA(const IntRect&);

  // Inflate an IntRect to account for its focus ring.
  // TODO: Consider using computing the focus ring's bounds with
  // -[NSCell focusRingMaskBoundsForFrame:inView:]).
  static PLATFORM_EXPORT IntRect InflateRectForFocusRing(const IntRect&);

  static PLATFORM_EXPORT LengthSize CheckboxSize(const FontDescription&,
                                                 const LengthSize& zoomed_size,
                                                 float zoom_factor);
  static PLATFORM_EXPORT NSButtonCell* Checkbox(ControlStates,
                                                const IntRect& zoomed_rect,
                                                float zoom_factor);
  static PLATFORM_EXPORT const IntSize* CheckboxSizes();
  static PLATFORM_EXPORT const int* CheckboxMargins(NSControlSize);
  static PLATFORM_EXPORT NSView* EnsuredView(ScrollableArea*);

  static PLATFORM_EXPORT const IntSize* RadioSizes();
  static PLATFORM_EXPORT const int* RadioMargins(NSControlSize);
  static PLATFORM_EXPORT LengthSize RadioSize(const FontDescription&,
                                              const LengthSize& zoomed_size,
                                              float zoom_factor);
  static PLATFORM_EXPORT NSButtonCell* Radio(ControlStates,
                                             const IntRect& zoomed_rect,
                                             float zoom_factor);

  static PLATFORM_EXPORT const IntSize* ButtonSizes();
  static PLATFORM_EXPORT const int* ButtonMargins(NSControlSize);
  static PLATFORM_EXPORT NSButtonCell* Button(ControlPart,
                                              ControlStates,
                                              const IntRect& zoomed_rect,
                                              float zoom_factor);

  static PLATFORM_EXPORT NSControlSize
  ControlSizeFromPixelSize(const IntSize* sizes,
                           const IntSize& min_zoomed_size,
                           float zoom_factor);
  static PLATFORM_EXPORT const IntSize* StepperSizes();
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_MAC_THEME_MAC_H_
