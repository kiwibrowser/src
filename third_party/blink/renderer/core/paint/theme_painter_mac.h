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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_THEME_PAINTER_MAC_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_THEME_PAINTER_MAC_H_

#import "third_party/blink/renderer/core/paint/theme_painter.h"

namespace blink {

class LayoutThemeMac;

class ThemePainterMac final : public ThemePainter {
 public:
  ThemePainterMac(LayoutThemeMac&);

 private:
  bool PaintButton(const Node*,
                   const Document&,
                   const ComputedStyle&,
                   const PaintInfo&,
                   const IntRect&) override;
  bool PaintCheckbox(const Node*,
                     const Document&,
                     const ComputedStyle&,
                     const PaintInfo&,
                     const IntRect&) override;
  bool PaintCapsLockIndicator(const LayoutObject&,
                              const PaintInfo&,
                              const IntRect&) override;
  bool PaintInnerSpinButton(const Node*,
                            const ComputedStyle&,
                            const PaintInfo&,
                            const IntRect&) override;
  bool PaintMenuList(const Node*,
                     const Document&,
                     const ComputedStyle&,
                     const PaintInfo&,
                     const IntRect&) override;
  bool PaintMenuListButton(const Node*,
                           const Document&,
                           const ComputedStyle&,
                           const PaintInfo&,
                           const IntRect&) override;
  bool PaintProgressBar(const LayoutObject&,
                        const PaintInfo&,
                        const IntRect&) override;
  bool PaintRadio(const Node*,
                  const Document&,
                  const ComputedStyle&,
                  const PaintInfo&,
                  const IntRect&) override;
  bool PaintSliderThumb(const Node*,
                        const ComputedStyle&,
                        const PaintInfo&,
                        const IntRect&) override;
  bool PaintSliderTrack(const LayoutObject&,
                        const PaintInfo&,
                        const IntRect&) override;
  bool PaintSearchField(const Node*,
                        const ComputedStyle&,
                        const PaintInfo&,
                        const IntRect&) override;
  bool PaintSearchFieldCancelButton(const LayoutObject&,
                                    const PaintInfo&,
                                    const IntRect&) override;
  bool PaintTextArea(const Node*,
                     const ComputedStyle&,
                     const PaintInfo&,
                     const IntRect&) override;
  bool PaintTextField(const Node*,
                      const ComputedStyle&,
                      const PaintInfo&,
                      const IntRect&) override;

  LayoutThemeMac& layout_theme_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_THEME_PAINTER_MAC_H_
