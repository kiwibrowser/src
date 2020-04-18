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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_SCROLLBAR_THEME_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_SCROLLBAR_THEME_H_

#include "third_party/blink/renderer/platform/scroll/scrollbar_theme.h"

namespace blink {

class WebMouseEvent;

class LayoutScrollbarTheme final : public ScrollbarTheme {
 public:
  ~LayoutScrollbarTheme() override = default;

  int ScrollbarThickness(ScrollbarControlSize control_size) override {
    return ScrollbarTheme::DeprecatedStaticGetTheme().ScrollbarThickness(
        control_size);
  }

  WebScrollbarButtonsPlacement ButtonsPlacement() const override {
    return ScrollbarTheme::DeprecatedStaticGetTheme().ButtonsPlacement();
  }

  void PaintScrollCorner(GraphicsContext&,
                         const DisplayItemClient&,
                         const IntRect& corner_rect) override;

  bool ShouldCenterOnThumb(const Scrollbar& scrollbar,
                           const WebMouseEvent& event) override {
    return ScrollbarTheme::DeprecatedStaticGetTheme().ShouldCenterOnThumb(
        scrollbar, event);
  }
  bool ShouldSnapBackToDragOrigin(const Scrollbar& scrollbar,
                                  const WebMouseEvent& event) override {
    return ScrollbarTheme::DeprecatedStaticGetTheme()
        .ShouldSnapBackToDragOrigin(scrollbar, event);
  }

  double InitialAutoscrollTimerDelay() override {
    return ScrollbarTheme::DeprecatedStaticGetTheme()
        .InitialAutoscrollTimerDelay();
  }
  double AutoscrollTimerDelay() override {
    return ScrollbarTheme::DeprecatedStaticGetTheme().AutoscrollTimerDelay();
  }

  void RegisterScrollbar(Scrollbar& scrollbar) override {
    return ScrollbarTheme::DeprecatedStaticGetTheme().RegisterScrollbar(
        scrollbar);
  }
  void UnregisterScrollbar(Scrollbar& scrollbar) override {
    return ScrollbarTheme::DeprecatedStaticGetTheme().UnregisterScrollbar(
        scrollbar);
  }

  int MinimumThumbLength(const Scrollbar&) override;

  void ButtonSizesAlongTrackAxis(const Scrollbar&,
                                 int& before_size,
                                 int& after_size);

  static LayoutScrollbarTheme* GetLayoutScrollbarTheme();

 protected:
  bool HasButtons(const Scrollbar&) override;
  bool HasThumb(const Scrollbar&) override;

  IntRect BackButtonRect(const Scrollbar&,
                         ScrollbarPart,
                         bool painting = false) override;
  IntRect ForwardButtonRect(const Scrollbar&,
                            ScrollbarPart,
                            bool painting = false) override;
  IntRect TrackRect(const Scrollbar&, bool painting = false) override;

  void PaintScrollbarBackground(GraphicsContext&, const Scrollbar&) override;
  void PaintTrackBackground(GraphicsContext&,
                            const Scrollbar&,
                            const IntRect&) override;
  void PaintTrackPiece(GraphicsContext&,
                       const Scrollbar&,
                       const IntRect&,
                       ScrollbarPart) override;
  void PaintButton(GraphicsContext&,
                   const Scrollbar&,
                   const IntRect&,
                   ScrollbarPart) override;
  void PaintThumb(GraphicsContext&, const Scrollbar&, const IntRect&) override;
  void PaintTickmarks(GraphicsContext&,
                      const Scrollbar&,
                      const IntRect&) override;

  IntRect ConstrainTrackRectToTrackPieces(const Scrollbar&,
                                          const IntRect&) override;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_LAYOUT_SCROLLBAR_THEME_H_
