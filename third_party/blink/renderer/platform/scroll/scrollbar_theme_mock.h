/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCROLL_SCROLLBAR_THEME_MOCK_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCROLL_SCROLLBAR_THEME_MOCK_H_

#include "third_party/blink/renderer/platform/scroll/scrollbar_theme.h"

namespace blink {

// Scrollbar theme used in image snapshots, to eliminate appearance differences
// between platforms.
class PLATFORM_EXPORT ScrollbarThemeMock : public ScrollbarTheme {
 public:
  int ScrollbarThickness(ScrollbarControlSize = kRegularScrollbar) override;
  bool UsesOverlayScrollbars() const override;

 protected:
  bool HasButtons(const Scrollbar&) override { return false; }
  bool HasThumb(const Scrollbar&) override { return true; }

  IntRect BackButtonRect(const Scrollbar&,
                         ScrollbarPart,
                         bool /*painting*/ = false) override {
    return IntRect();
  }
  IntRect ForwardButtonRect(const Scrollbar&,
                            ScrollbarPart,
                            bool /*painting*/ = false) override {
    return IntRect();
  }
  IntRect TrackRect(const Scrollbar&, bool painting = false) override;

  void PaintTrackBackground(GraphicsContext&,
                            const Scrollbar&,
                            const IntRect&) override;
  void PaintThumb(GraphicsContext&, const Scrollbar&, const IntRect&) override;

  void PaintScrollCorner(GraphicsContext&,
                         const DisplayItemClient&,
                         const IntRect& corner_rect) override;

  int MinimumThumbLength(const Scrollbar&) override;

 private:
  bool IsMockTheme() const final { return true; }
};

}  // namespace blink
#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCROLL_SCROLLBAR_THEME_MOCK_H_
