/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_SHAPING_SHAPE_RESULT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_SHAPING_SHAPE_RESULT_H_

#include <memory>
#include "third_party/blink/renderer/platform/fonts/canvas_rotation_in_vertical.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/layout_unit.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/text/text_direction.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

struct hb_buffer_t;

namespace blink {

struct CharacterRange;
class Font;
template <typename TextContainerType>
class PLATFORM_EXPORT ShapeResultSpacing;
class SimpleFontData;
class TextRun;

enum class AdjustMidCluster {
  // Adjust the middle of a grapheme cluster to the logical end boundary.
  kToEnd,
  // Adjust the middle of a grapheme cluster to the logical start boundary.
  kToStart
};

class PLATFORM_EXPORT ShapeResult : public RefCounted<ShapeResult> {
 public:
  static scoped_refptr<ShapeResult> Create(const Font* font,
                                    unsigned num_characters,
                                    TextDirection direction) {
    return base::AdoptRef(new ShapeResult(font, num_characters, direction));
  }
  static scoped_refptr<ShapeResult> CreateForTabulationCharacters(
      const Font*,
      const TextRun&,
      float position_offset,
      unsigned count);
  ~ShapeResult();

  // Returns a mutable unique instance. If |this| has more than 1 ref count,
  // a clone is created.
  scoped_refptr<ShapeResult> MutableUnique() const;

  // The logical width of this result.
  float Width() const { return width_; }
  LayoutUnit SnappedWidth() const { return LayoutUnit::FromFloatCeil(width_); }
  // The glyph bounding box, in logical coordinates, using alphabetic baseline
  // even when the result is in vertical flow.
  const FloatRect& Bounds() const { return glyph_bounding_box_; }
  unsigned NumCharacters() const { return num_characters_; }
  CharacterRange GetCharacterRange(unsigned from, unsigned to) const;
  // The character start/end index of a range shape result.
  unsigned StartIndexForResult() const;
  unsigned EndIndexForResult() const;
  void FallbackFonts(HashSet<const SimpleFontData*>*) const;
  TextDirection Direction() const {
    return static_cast<TextDirection>(direction_);
  }
  bool Rtl() const { return Direction() == TextDirection::kRtl; }

  // True if at least one glyph in this result has vertical offsets.
  //
  // Vertical result always has vertical offsets, but horizontal result may also
  // have vertical offsets.
  bool HasVerticalOffsets() const { return has_vertical_offsets_; }

  // For memory reporting.
  size_t ByteSize() const;

  // Returns the next or previous offsets respectively at which it is safe to
  // break without reshaping.
  // The |offset| given and the return value is for the original string, between
  // |StartIndexForResult| and |EndIndexForResult|.
  unsigned NextSafeToBreakOffset(unsigned offset) const;
  unsigned PreviousSafeToBreakOffset(unsigned offset) const;

  // Returns the offset whose (origin, origin+advance) contains |x|.
  unsigned OffsetForPosition(float x) const;
  // Returns the offset whose glyph boundary is nearest to |x|. Depends on
  // whether |x| is on the left-half or the right-half of the glyph, it
  // determines the left-boundary or the right-boundary, then computes the
  // offset from the bidi direction.
  unsigned OffsetForHitTest(float x) const;
  // Returns the offset that can fit to between |x| and the left or the right
  // edge. The side of the edge is determined by |line_direction|.
  unsigned OffsetToFit(float x, TextDirection line_direction) const;
  unsigned OffsetForPosition(float x, bool include_partial_glyphs) const {
    return !include_partial_glyphs ? OffsetForPosition(x) : OffsetForHitTest(x);
  }

  float PositionForOffset(unsigned offset,
                          AdjustMidCluster = AdjustMidCluster::kToEnd) const;
  LayoutUnit SnappedStartPositionForOffset(unsigned offset) const {
    return LayoutUnit::FromFloatFloor(PositionForOffset(offset));
  }
  LayoutUnit SnappedEndPositionForOffset(unsigned offset) const {
    return LayoutUnit::FromFloatCeil(PositionForOffset(offset));
  }

  // Apply spacings (letter-spacing, word-spacing, and justification) as
  // configured to |ShapeResultSpacing|.
  // |text_start_offset| adjusts the character index in the ShapeResult before
  // giving it to |ShapeResultSpacing|. It can be negative if
  // |StartIndexForResult()| is larger than the text in |ShapeResultSpacing|.
  void ApplySpacing(ShapeResultSpacing<String>&, int text_start_offset = 0);
  scoped_refptr<ShapeResult> ApplySpacingToCopy(ShapeResultSpacing<TextRun>&,
                                         const TextRun&) const;

  // Append a copy of a range within an existing result to another result.
  void CopyRange(unsigned start, unsigned end, ShapeResult*) const;

  // Create a new ShapeResult instance from a range within an existing result.
  scoped_refptr<ShapeResult> SubRange(unsigned start_offset,
                                      unsigned end_offset) const;

  // Create a new ShapeResult instance with the start offset adjusted.
  scoped_refptr<ShapeResult> CopyAdjustedOffset(unsigned start_offset) const;

  // Computes the list of fonts along with the number of glyphs for each font.
  struct RunFontData {
    SimpleFontData* font_data_;
    size_t glyph_count_;
  };
  void GetRunFontData(Vector<RunFontData>* font_data) const;

  String ToString() const;
  void ToString(StringBuilder*) const;

  struct RunInfo;
  RunInfo* InsertRunForTesting(unsigned start_index,
                               unsigned num_characters,
                               TextDirection,
                               Vector<uint16_t> safe_break_offsets = {});
#if DCHECK_IS_ON()
  void CheckConsistency() const;
#endif

 protected:
  ShapeResult(const SimpleFontData*, unsigned num_characters, TextDirection);
  ShapeResult(const Font*, unsigned num_characters, TextDirection);
  ShapeResult(const ShapeResult&);

  static scoped_refptr<ShapeResult> Create(const SimpleFontData* font_data,
                                           unsigned num_characters,
                                           TextDirection direction) {
    return base::AdoptRef(
        new ShapeResult(font_data, num_characters, direction));
  }
  static scoped_refptr<ShapeResult> Create(const ShapeResult& other) {
    return base::AdoptRef(new ShapeResult(other));
  }

  struct GlyphIndexResult {
    STACK_ALLOCATED();

    unsigned run_index = 0;
    // The total number of characters of runs_[0..run_index - 1].
    unsigned characters_on_left_runs = 0;
    unsigned character_index = 0;
    unsigned glyph_index = 0;
    // |next_glyph_index| may not be |glyph_index| + 1 when a cluster is of
    // multiple glyphs; i.e., ligatures or combining glyphs.
    unsigned next_glyph_index = 0;
    // The glyph origin of the glyph.
    float origin_x = 0;
    // The advance of the glyph.
    float advance = 0;

    // True if the position was found on a run. False otherwise.
    bool IsInRun() const { return next_glyph_index; }
  };

  unsigned OffsetLtr(const GlyphIndexResult&) const;
  unsigned OffsetRtl(const GlyphIndexResult&, float x) const;
  unsigned OffsetRightLtr(const GlyphIndexResult&) const;
  unsigned OffsetLeftRtl(const GlyphIndexResult&) const;

  void OffsetForPosition(float target_x, GlyphIndexResult*) const;

  template <typename TextContainerType>
  void ApplySpacingImpl(ShapeResultSpacing<TextContainerType>&,
                        int text_start_offset = 0);
  template <bool is_horizontal_run>
  void ComputeGlyphPositions(ShapeResult::RunInfo*,
                             unsigned start_glyph,
                             unsigned num_glyphs,
                             hb_buffer_t*);
  void InsertRun(std::unique_ptr<ShapeResult::RunInfo>,
                 unsigned start_glyph,
                 unsigned num_glyphs,
                 hb_buffer_t*);
  void InsertRun(std::unique_ptr<ShapeResult::RunInfo>);
  void InsertRunForIndex(unsigned start_character_index);
  void ReorderRtlRuns(unsigned run_size_before);

  float LineLeftBounds() const;
  float LineRightBounds() const;

  float width_;
  FloatRect glyph_bounding_box_;
  Vector<std::unique_ptr<RunInfo>> runs_;
  scoped_refptr<const SimpleFontData> primary_font_;

  unsigned num_characters_;
  unsigned num_glyphs_ : 30;

  // Overall direction for the TextRun, dictates which order each individual
  // sub run (represented by RunInfo structs in the m_runs vector) can have a
  // different text direction.
  unsigned direction_ : 1;

  // Tracks whether any runs contain glyphs with a y-offset != 0.
  unsigned has_vertical_offsets_ : 1;

  friend class HarfBuzzShaper;
  friend class ShapeResultBuffer;
  friend class ShapeResultBloberizer;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_SHAPING_SHAPE_RESULT_H_
