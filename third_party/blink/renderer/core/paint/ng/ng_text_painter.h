// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_NG_NG_TEXT_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_NG_NG_TEXT_PAINTER_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/paint/text_painter_base.h"

namespace blink {

class NGPhysicalTextFragment;
struct NGTextFragmentPaintInfo;

// Text painter for LayoutNG, logic shared between legacy layout and LayoutNG
// is implemented in the TextPainterBase base class.
// Operates on NGPhysicalTextFragments and only paints text and decorations.
// Border painting etc is handled by the NGTextFragmentPainter class.
// TODO(layout-dev): Does this distinction make sense?
class CORE_EXPORT NGTextPainter : public TextPainterBase {
  STACK_ALLOCATED();

 public:
  NGTextPainter(GraphicsContext& context,
                const Font& font,
                const NGPhysicalTextFragment& text_fragment,
                const LayoutPoint& text_origin,
                const LayoutRect& text_bounds,
                bool horizontal)
      : TextPainterBase(context, font, text_origin, text_bounds, horizontal),
        fragment_(text_fragment) {}
  ~NGTextPainter() = default;

  void ClipDecorationsStripe(float upper,
                             float stripe_width,
                             float dilation) override;
  void Paint(unsigned start_offset,
             unsigned end_offset,
             unsigned length,
             const TextPaintStyle&);

  static TextPaintStyle TextPaintingStyle(const NGPhysicalTextFragment*,
                                          const ComputedStyle&,
                                          const PaintInfo&);
  static TextPaintStyle SelectionPaintingStyle(
      const NGPhysicalTextFragment*,
      bool have_selection,
      const PaintInfo&,
      const TextPaintStyle& text_style);

 private:
  template <PaintInternalStep step>
  void PaintInternalFragment(NGTextFragmentPaintInfo&,
                             unsigned from,
                             unsigned to);

  template <PaintInternalStep step>
  void PaintInternal(unsigned start_offset,
                     unsigned end_offset,
                     unsigned truncation_point);

  void PaintEmphasisMarkForCombinedText();

  const NGPhysicalTextFragment& fragment_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_NG_NG_TEXT_PAINTER_H_
