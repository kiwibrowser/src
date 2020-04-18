/*
 * Copyright (c) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#include "third_party/blink/renderer/platform/fonts/shaping/shape_result.h"

#include <hb.h>
#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include "third_party/blink/renderer/platform/fonts/character_range.h"
#include "third_party/blink/renderer/platform/fonts/font.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result_buffer.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result_inline_headers.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result_spacing.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {

unsigned ShapeResult::RunInfo::NextSafeToBreakOffset(unsigned offset) const {
  DCHECK_LE(offset, num_characters_);
  for (unsigned i = 0; i < safe_break_offsets_.size(); i++) {
    if (safe_break_offsets_[i] >= offset)
      return safe_break_offsets_[i];
  }

  // Next safe break is at the end of the run.
  return num_characters_;
}

unsigned ShapeResult::RunInfo::PreviousSafeToBreakOffset(
    unsigned offset) const {
  if (offset >= num_characters_)
    return num_characters_;

  for (unsigned i = safe_break_offsets_.size(); i > 0; i--) {
    if (safe_break_offsets_[i - 1] <= offset)
      return safe_break_offsets_[i - 1];
  }

  // Next safe break is at the start of the run.
  return 0;
}

float ShapeResult::RunInfo::XPositionForVisualOffset(
    unsigned offset,
    AdjustMidCluster adjust_mid_cluster) const {
  DCHECK_LT(offset, num_characters_);
  if (Rtl())
    offset = num_characters_ - offset - 1;
  return XPositionForOffset(offset, adjust_mid_cluster);
}

float ShapeResult::RunInfo::XPositionForOffset(
    unsigned offset,
    AdjustMidCluster adjust_mid_cluster) const {
  DCHECK_LE(offset, num_characters_);
  const unsigned num_glyphs = glyph_data_.size();
  unsigned glyph_index = 0;
  float position = 0;
  if (Rtl()) {
    while (glyph_index < num_glyphs &&
           glyph_data_[glyph_index].character_index > offset) {
      position += glyph_data_[glyph_index].advance;
      ++glyph_index;
    }
    // If |glyph_index| is at the end, the glyph for |offset| is missing, along
    // with all glyphs before it. We can't adjust position to the start
    // direction.
    if (glyph_index == num_glyphs)
      return position;
    // Adjust offset if it's not on the cluster boundary. In RTL, this means
    // that the adjusted position is the left side of the character.
    if (adjust_mid_cluster == AdjustMidCluster::kToEnd &&
        glyph_data_[glyph_index].character_index < offset) {
      return position;
    }
    // For RTL, we need to return the right side boundary of the character.
    // Add advance of glyphs which are part of the character.
    while (glyph_index < num_glyphs - 1 &&
           glyph_data_[glyph_index].character_index ==
               glyph_data_[glyph_index + 1].character_index) {
      position += glyph_data_[glyph_index].advance;
      ++glyph_index;
    }
    position += glyph_data_[glyph_index].advance;
  } else {
    while (glyph_index < num_glyphs &&
           glyph_data_[glyph_index].character_index < offset) {
      position += glyph_data_[glyph_index].advance;
      ++glyph_index;
    }
    // Adjust offset if it's not on the cluster boundary.
    if (adjust_mid_cluster == AdjustMidCluster::kToStart && glyph_index &&
        (glyph_index < num_glyphs ? glyph_data_[glyph_index].character_index
                                  : num_characters_) > offset) {
      offset = glyph_data_[--glyph_index].character_index;
      for (; glyph_data_[glyph_index].character_index == offset;
           --glyph_index) {
        position -= glyph_data_[glyph_index].advance;
        if (!glyph_index)
          break;
      }
    }
  }
  return position;
}

void ShapeResult::RunInfo::CharacterIndexForXPosition(
    float target_x,
    GlyphIndexResult* result) const {
  DCHECK(target_x >= 0 && target_x <= width_);
  const unsigned num_glyphs = glyph_data_.size();
  float current_x = 0;
  unsigned glyph_index = 0;

  while (true) {
    unsigned current_character_index = glyph_data_[glyph_index].character_index;
    float current_advance = glyph_data_[glyph_index].advance;
    unsigned next_glyph_index = glyph_index + 1;
    while (next_glyph_index < num_glyphs &&
           current_character_index ==
               glyph_data_[next_glyph_index].character_index)
      current_advance += glyph_data_[next_glyph_index++].advance;
    float next_x = current_x + current_advance;
    if (target_x < next_x || next_glyph_index == num_glyphs) {
      result->glyph_index = glyph_index;
      result->next_glyph_index = next_glyph_index;
      result->character_index = current_character_index;
      result->origin_x = current_x;
      result->advance = current_advance;
      return;
    }
    current_x = next_x;
    glyph_index = next_glyph_index;
  }
  NOTREACHED();
}

void HarfBuzzRunGlyphData::SetGlyphAndPositions(uint16_t glyph_id,
                                                uint16_t character_index,
                                                float advance,
                                                const FloatSize& offset) {
  glyph = glyph_id;
  this->character_index = character_index;
  this->advance = advance;
  this->offset = offset;
}

ShapeResult::ShapeResult(const SimpleFontData* font_data,
                         unsigned num_characters,
                         TextDirection direction)
    : width_(0),
      primary_font_(font_data),
      num_characters_(num_characters),
      num_glyphs_(0),
      direction_(static_cast<unsigned>(direction)),
      has_vertical_offsets_(0) {}

ShapeResult::ShapeResult(const Font* font,
                         unsigned num_characters,
                         TextDirection direction)
    : ShapeResult(font->PrimaryFont(), num_characters, direction) {}

ShapeResult::ShapeResult(const ShapeResult& other)
    : width_(other.width_),
      glyph_bounding_box_(other.glyph_bounding_box_),
      primary_font_(other.primary_font_),
      num_characters_(other.num_characters_),
      num_glyphs_(other.num_glyphs_),
      direction_(other.direction_),
      has_vertical_offsets_(other.has_vertical_offsets_) {
  runs_.ReserveCapacity(other.runs_.size());
  for (const auto& run : other.runs_)
    runs_.push_back(std::make_unique<RunInfo>(*run));
}

ShapeResult::~ShapeResult() = default;

size_t ShapeResult::ByteSize() const {
  size_t self_byte_size = sizeof(this);
  for (unsigned i = 0; i < runs_.size(); ++i) {
    self_byte_size += runs_[i]->ByteSize();
  }
  return self_byte_size;
}

CharacterRange ShapeResult::GetCharacterRange(unsigned from,
                                              unsigned to) const {
  return ShapeResultBuffer::GetCharacterRange(this, Direction(), Width(), from,
                                              to);
}

unsigned ShapeResult::StartIndexForResult() const {
  if (UNLIKELY(runs_.IsEmpty()))
    return 0;
  const RunInfo& first_run = *runs_.front();
  if (!Rtl())
    return first_run.start_index_;
  unsigned end = first_run.start_index_ + first_run.num_characters_;
  DCHECK_GE(end, NumCharacters());
  return end - NumCharacters();
}

unsigned ShapeResult::EndIndexForResult() const {
  if (UNLIKELY(runs_.IsEmpty()))
    return NumCharacters();
  const RunInfo& first_run = *runs_.front();
  if (!Rtl())
    return first_run.start_index_ + NumCharacters();
  return first_run.start_index_ + first_run.num_characters_;
}

scoped_refptr<ShapeResult> ShapeResult::MutableUnique() const {
  if (HasOneRef())
    return const_cast<ShapeResult*>(this);
  return ShapeResult::Create(*this);
}

unsigned ShapeResult::NextSafeToBreakOffset(unsigned index) const {
  for (auto* it = runs_.begin(); it != runs_.end(); ++it) {
    const auto& run = *it;
    if (!run)
      continue;

    unsigned run_start = run->start_index_;
    if (index >= run_start) {
      unsigned offset = index - run_start;
      if (offset <= run->num_characters_) {
        return run->NextSafeToBreakOffset(offset) + run_start;
      }
      if (Rtl()) {
        if (it == runs_.begin())
          return run_start + run->num_characters_;
        const auto& previous_run = *--it;
        return previous_run->start_index_;
      }
    } else if (!Rtl()) {
      return run_start;
    }
  }

  return EndIndexForResult();
}

unsigned ShapeResult::PreviousSafeToBreakOffset(unsigned index) const {
  for (auto it = runs_.rbegin(); it != runs_.rend(); ++it) {
    const auto& run = *it;
    if (!run)
      continue;

    unsigned run_start = run->start_index_;
    if (index >= run_start) {
      unsigned offset = index - run_start;
      if (offset <= run->num_characters_) {
        return run->PreviousSafeToBreakOffset(offset) + run_start;
      }
      if (!Rtl()) {
        return run_start + run->num_characters_;
      }
    } else if (Rtl()) {
      if (it == runs_.rbegin())
        return run->start_index_;
      const auto& previous_run = *--it;
      return previous_run->start_index_ + previous_run->num_characters_;
    }
  }

  return StartIndexForResult();
}

// Returns the offset of the character of |result| for LTR.
unsigned ShapeResult::OffsetLtr(const GlyphIndexResult& result) const {
  DCHECK(IsLtr(Direction()));
  return result.characters_on_left_runs + result.character_index;
}

// Returns the offset of the character of |result| for RTL.
unsigned ShapeResult::OffsetRtl(const GlyphIndexResult& result, float x) const {
  DCHECK(IsRtl(Direction()));
  if (!result.IsInRun())
    return NumCharacters() - result.characters_on_left_runs;
  // In RTL, the boundary belongs to the left character. This subtle difference
  // allows round trips between OffsetForPoint and PointForOffset.
  if (UNLIKELY(x == result.origin_x))
    return OffsetLeftRtl(result);
  return NumCharacters() - result.characters_on_left_runs -
         runs_[result.run_index]->num_characters_ + result.character_index;
}

// Returns the offset of the character on the right of |result| for LTR.
unsigned ShapeResult::OffsetRightLtr(const GlyphIndexResult& result) const {
  DCHECK(IsLtr(Direction()));
  if (result.run_index >= runs_.size())
    return NumCharacters();
  const RunInfo& run = *runs_[result.run_index];
  return result.characters_on_left_runs +
         (result.next_glyph_index < run.glyph_data_.size()
              ? run.glyph_data_[result.next_glyph_index].character_index
              : run.num_characters_);
}

// Returns the offset of the character on the left of |result| for RTL.
unsigned ShapeResult::OffsetLeftRtl(const GlyphIndexResult& result) const {
  DCHECK(IsRtl(Direction()));
  if (!result.glyph_index)
    return NumCharacters() - result.characters_on_left_runs;
  const RunInfo& run = *runs_[result.run_index];
  return NumCharacters() - result.characters_on_left_runs -
         run.num_characters_ +
         run.glyph_data_[result.glyph_index - 1].character_index;
}

// If the position is outside of the result, returns the start or the end offset
// depends on the position.
void ShapeResult::OffsetForPosition(float target_x,
                                    GlyphIndexResult* result) const {
  if (target_x <= 0)
    return;

  unsigned characters_so_far = 0;
  float current_x = 0;
  for (unsigned i = 0; i < runs_.size(); ++i) {
    const RunInfo* run = runs_[i].get();
    if (!run)
      continue;
    float next_x = current_x + run->width_;
    float offset_for_run = target_x - current_x;
    if (offset_for_run >= 0 && offset_for_run < run->width_) {
      // The x value in question is within this script run.
      run->CharacterIndexForXPosition(offset_for_run, result);
      result->run_index = i;
      result->characters_on_left_runs = characters_so_far;
      result->origin_x += current_x;
      DCHECK_LE(result->characters_on_left_runs + result->character_index,
                NumCharacters());
      return;
    }
    characters_so_far += run->num_characters_;
    current_x = next_x;
  }

  result->run_index = runs_.size();
  result->characters_on_left_runs = characters_so_far;
}

unsigned ShapeResult::OffsetForPosition(float x) const {
  GlyphIndexResult result;
  OffsetForPosition(x, &result);
  return IsLtr(Direction()) ? OffsetLtr(result) : OffsetRtl(result, x);
}

unsigned ShapeResult::OffsetForHitTest(float x) const {
  GlyphIndexResult result;
  OffsetForPosition(x, &result);
  if (IsLtr(Direction())) {
    if (result.IsInRun() && x > result.origin_x + result.advance / 2)
      return OffsetRightLtr(result);
    return OffsetLtr(result);
  }
  if (result.IsInRun() && x <= result.origin_x + result.advance / 2)
    return OffsetLeftRtl(result);
  return OffsetRtl(result, x);
}

unsigned ShapeResult::OffsetToFit(float x, TextDirection line_direction) const {
  GlyphIndexResult result;
  OffsetForPosition(x, &result);
  if (IsLtr(line_direction)) {
    return IsLtr(Direction()) ? OffsetLtr(result) : OffsetLeftRtl(result);
  }
  return IsRtl(Direction()) ? OffsetRtl(result, x) : OffsetRightLtr(result);
}

float ShapeResult::PositionForOffset(
    unsigned absolute_offset,
    AdjustMidCluster adjust_mid_cluster) const {
  float x = 0;
  float offset_x = 0;

  // The absolute_offset argument represents the offset for the entire
  // ShapeResult while offset is continuously updated to be relative to the
  // current run.
  unsigned offset = absolute_offset;

  if (Rtl()) {
    // Convert logical offsets to visual offsets, because results are in
    // logical order while runs are in visual order.
    x = width_;
    if (offset < NumCharacters())
      offset = NumCharacters() - offset - 1;
    x -= Width();
  }

  for (unsigned i = 0; i < runs_.size(); i++) {
    if (!runs_[i])
      continue;
    DCHECK_EQ(Rtl(), runs_[i]->Rtl());
    unsigned num_characters = runs_[i]->num_characters_;

    if (!offset_x && offset < num_characters) {
      offset_x =
          runs_[i]->XPositionForVisualOffset(offset, adjust_mid_cluster) + x;
      break;
    }

    offset -= num_characters;
    x += runs_[i]->width_;
  }

  // The position in question might be just after the text.
  if (!offset_x && absolute_offset == NumCharacters())
    return Rtl() ? 0 : width_;

  return offset_x;
}

void ShapeResult::FallbackFonts(
    HashSet<const SimpleFontData*>* fallback) const {
  DCHECK(fallback);
  DCHECK(primary_font_);
  for (unsigned i = 0; i < runs_.size(); ++i) {
    if (runs_[i] && runs_[i]->font_data_ &&
        runs_[i]->font_data_ != primary_font_) {
      fallback->insert(runs_[i]->font_data_.get());
    }
  }
}

void ShapeResult::GetRunFontData(Vector<RunFontData>* font_data) const {
  for (const auto& run : runs_) {
    font_data->push_back(
        RunFontData({run->font_data_.get(), run->glyph_data_.size()}));
  }
}

// TODO(kojii): VC2015 fails to explicit instantiation of a member function.
// Typed functions + this private function are to instantiate instances.
template <typename TextContainerType>
void ShapeResult::ApplySpacingImpl(
    ShapeResultSpacing<TextContainerType>& spacing,
    int text_start_offset) {
  float offset = 0;
  float total_space = 0;
  float space = 0;
  for (auto& run : runs_) {
    if (!run)
      continue;
    unsigned run_start_index = run->start_index_ + text_start_offset;
    float total_space_for_run = 0;
    for (size_t i = 0; i < run->glyph_data_.size(); i++) {
      HarfBuzzRunGlyphData& glyph_data = run->glyph_data_[i];

      // Skip if it's not a grapheme cluster boundary.
      if (i + 1 < run->glyph_data_.size() &&
          glyph_data.character_index ==
              run->glyph_data_[i + 1].character_index) {
        continue;
      }

      space = spacing.ComputeSpacing(
          run_start_index + glyph_data.character_index, offset);
      glyph_data.advance += space;
      total_space_for_run += space;

      // |offset| is non-zero only when justifying CJK characters that follow
      // non-CJK characters.
      if (UNLIKELY(offset)) {
        if (run->IsHorizontal()) {
          glyph_data.offset.SetWidth(glyph_data.offset.Width() + offset);
        } else {
          glyph_data.offset.SetHeight(glyph_data.offset.Height() + offset);
          has_vertical_offsets_ = true;
        }
        offset = 0;
      }
    }
    run->width_ += total_space_for_run;
    total_space += total_space_for_run;
  }
  width_ += total_space;

  // The spacing on the right of the last glyph does not affect the glyph
  // bounding box. Thus, the glyph bounding box becomes smaller than the advance
  // if the letter spacing is positve, or larger if negative.
  if (space) {
    total_space -= space;

    // TODO(kojii): crbug.com/768284: There are cases where
    // InlineTextBox::LogicalWidth() is round down of ShapeResult::Width() in
    // LayoutUnit. Ceiling the width did not help. Add 1px to avoid cut-off.
    if (space < 0)
      total_space += 1;
  }

  // Set the width because glyph bounding box is in logical space.
  float glyph_bounding_box_width = glyph_bounding_box_.Width() + total_space;
  if (width_ >= 0 && glyph_bounding_box_width >= 0) {
    glyph_bounding_box_.SetWidth(glyph_bounding_box_width);
    return;
  }

  // Negative word-spacing and/or letter-spacing may cause some glyphs to
  // overflow the left boundary and result negative measured width. Adjust glyph
  // bounds accordingly to cover the overflow.
  // The negative width should be clamped to 0 in CSS box model, but it's up to
  // caller's responsibility.
  float left = std::min(width_, glyph_bounding_box_width);
  if (left < glyph_bounding_box_.X()) {
    // The right edge should be the width of the first character in most cases,
    // but computing it requires re-measuring bounding box of each glyph. Leave
    // it unchanged, which gives an excessive right edge but assures it covers
    // all glyphs.
    glyph_bounding_box_.ShiftXEdgeTo(left);
  } else {
    glyph_bounding_box_.SetWidth(glyph_bounding_box_width);
  }
}

void ShapeResult::ApplySpacing(ShapeResultSpacing<String>& spacing,
                               int text_start_offset) {
  ApplySpacingImpl(spacing, text_start_offset);
}

scoped_refptr<ShapeResult> ShapeResult::ApplySpacingToCopy(
    ShapeResultSpacing<TextRun>& spacing,
    const TextRun& run) const {
  unsigned index_of_sub_run = spacing.Text().IndexOfSubRun(run);
  DCHECK_NE(std::numeric_limits<unsigned>::max(), index_of_sub_run);
  scoped_refptr<ShapeResult> result = ShapeResult::Create(*this);
  if (index_of_sub_run != std::numeric_limits<unsigned>::max())
    result->ApplySpacingImpl(spacing, index_of_sub_run);
  return result;
}

namespace {

float HarfBuzzPositionToFloat(hb_position_t value) {
  return static_cast<float>(value) / (1 << 16);
}

// This is a helper class to accumulate glyph bounding box.
//
// Glyph positions and bounding boxes from HarfBuzz and fonts are in physical
// coordinate, while ShapeResult::glyph_bounding_box_ is in logical coordinate.
// To minimize the number of conversions, this class accumulates the bounding
// boxes in physical coordinate, and convert the accumulated box to logical.
struct GlyphBoundsAccumulator {
  // Construct an accumulator with the logical glyph origin.
  explicit GlyphBoundsAccumulator(float origin) : origin(origin) {}

  // The accumulated glyph bounding box in physical coordinate, until
  // ConvertVerticalRunToLogical().
  FloatRect bounds;
  // The current origin, in logical coordinate.
  float origin;

  // Unite a glyph bounding box to |bounds|.
  template <bool is_horizontal_run>
  void Unite(const HarfBuzzRunGlyphData& glyph_data,
             FloatRect bounds_for_glyph) {
    if (UNLIKELY(bounds_for_glyph.IsEmpty()))
      return;

    // Glyphs are drawn at |origin + offset|. Move glyph_bounds to that point.
    // All positions in hb_glyph_position_t are relative to the current point.
    // https://behdad.github.io/harfbuzz/harfbuzz-Buffers.html#hb-glyph-position-t-struct
    if (is_horizontal_run)
      bounds_for_glyph.SetX(bounds_for_glyph.X() + origin);
    else
      bounds_for_glyph.SetY(bounds_for_glyph.Y() + origin);
    bounds_for_glyph.Move(glyph_data.offset);

    bounds.Unite(bounds_for_glyph);
  }

  // Non-template version of |Unite()|, see above.
  void Unite(bool is_horizontal_run,
             const HarfBuzzRunGlyphData& glyph,
             FloatRect bounds_for_glyph) {
    is_horizontal_run ? Unite<true>(glyph, bounds_for_glyph)
                      : Unite<false>(glyph, bounds_for_glyph);
  }

  // Convert vertical run glyph bounding box to logical. Horizontal runs do not
  // need conversions because physical and logical are the same.
  void ConvertVerticalRunToLogical(const FontMetrics& font_metrics) {
    // Convert physical glyph_bounding_box to logical.
    bounds = bounds.TransposedRect();

    // The glyph bounding box of a vertical run uses ideographic baseline.
    // Adjust the box Y position because the bounding box of a ShapeResult uses
    // alphabetic baseline.
    // See diagrams of base lines at
    // https://drafts.csswg.org/css-writing-modes-3/#intro-baselines
    int baseline_adjust = font_metrics.Ascent(kIdeographicBaseline) -
                          font_metrics.Ascent(kAlphabeticBaseline);
    bounds.SetY(bounds.Y() + baseline_adjust);
  }
};

// Checks whether it's safe to break without reshaping before the given glyph.
bool IsSafeToBreakBefore(const hb_glyph_info_t* glyph_infos,
                         unsigned num_glyphs,
                         unsigned i) {
  // Before the first glyph is safe to break.
  if (!i)
    return true;

  // Not at a cluster boundary.
  if (glyph_infos[i].cluster == glyph_infos[i - 1].cluster)
    return false;

  // The HB_GLYPH_FLAG_UNSAFE_TO_BREAK flag is set for all glyphs in a
  // given cluster so we only need to check the last one.
  hb_glyph_flags_t flags = hb_glyph_info_get_glyph_flags(glyph_infos + i);
  return (flags & HB_GLYPH_FLAG_UNSAFE_TO_BREAK) == 0;
}

}  // anonymous namespace

// Computes glyph positions, sets advance and offset of each glyph to RunInfo.
//
// Also computes glyph bounding box of the run. In this function, glyph bounding
// box is in physical.
template <bool is_horizontal_run>
void ShapeResult::ComputeGlyphPositions(ShapeResult::RunInfo* run,
                                        unsigned start_glyph,
                                        unsigned num_glyphs,
                                        hb_buffer_t* harf_buzz_buffer) {
  DCHECK_EQ(is_horizontal_run, run->IsHorizontal());
  const SimpleFontData& current_font_data = *run->font_data_;
  const hb_glyph_info_t* glyph_infos =
      hb_buffer_get_glyph_infos(harf_buzz_buffer, nullptr);
  const hb_glyph_position_t* glyph_positions =
      hb_buffer_get_glyph_positions(harf_buzz_buffer, nullptr);
  const unsigned start_cluster =
      HB_DIRECTION_IS_FORWARD(hb_buffer_get_direction(harf_buzz_buffer))
          ? glyph_infos[start_glyph].cluster
          : glyph_infos[start_glyph + num_glyphs - 1].cluster;

  // Compute glyph_origin and glyph_bounding_box in physical, since both offsets
  // and boudning box of glyphs are in physical. It's the caller's
  // responsibility to convert the united physical bounds to logical.
  float total_advance = 0.0f;
  GlyphBoundsAccumulator bounds(width_);
  bool has_vertical_offsets = !is_horizontal_run;

  // Because we reverse this later, it must be empty at this point.
  DCHECK(run->safe_break_offsets_.IsEmpty());

  // HarfBuzz returns result in visual order, no need to flip for RTL.
  for (unsigned i = 0; i < num_glyphs; ++i) {
    uint16_t glyph = glyph_infos[start_glyph + i].codepoint;
    const hb_glyph_position_t& pos = glyph_positions[start_glyph + i];

    // Offset is primarily used when painting glyphs. Keep it in physical.
    FloatSize offset(HarfBuzzPositionToFloat(pos.x_offset),
                     -HarfBuzzPositionToFloat(pos.y_offset));

    // One out of x_advance and y_advance is zero, depending on
    // whether the buffer direction is horizontal or vertical.
    // Convert to float and negate to avoid integer-overflow for ULONG_MAX.
    float advance = is_horizontal_run ? HarfBuzzPositionToFloat(pos.x_advance)
                                      : -HarfBuzzPositionToFloat(pos.y_advance);

    uint16_t character_index =
        glyph_infos[start_glyph + i].cluster - start_cluster;
    HarfBuzzRunGlyphData& glyph_data = run->glyph_data_[i];
    glyph_data.SetGlyphAndPositions(glyph, character_index, advance, offset);
    total_advance += advance;
    has_vertical_offsets |= (offset.Height() != 0);

    bounds.Unite<is_horizontal_run>(
        glyph_data, current_font_data.BoundsForGlyph(glyph_data.glyph));
    bounds.origin += advance;

    // Check if it is safe to break without reshaping before the cluster.
    if (IsSafeToBreakBefore(glyph_infos + start_glyph, num_glyphs, i))
      run->safe_break_offsets_.push_back(character_index);
  }

  run->width_ = std::max(0.0f, total_advance);
  has_vertical_offsets_ |= has_vertical_offsets;

  if (!is_horizontal_run)
    bounds.ConvertVerticalRunToLogical(current_font_data.GetFontMetrics());
  glyph_bounding_box_.Unite(bounds.bounds);

  if (UNLIKELY(run->Rtl()))
    run->safe_break_offsets_.Reverse();
}

void ShapeResult::InsertRun(std::unique_ptr<ShapeResult::RunInfo> run_to_insert,
                            unsigned start_glyph,
                            unsigned num_glyphs,
                            hb_buffer_t* harf_buzz_buffer) {
  DCHECK_GT(num_glyphs, 0u);
  std::unique_ptr<ShapeResult::RunInfo> run(std::move(run_to_insert));
  DCHECK_EQ(num_glyphs, run->glyph_data_.size());

  if (run->IsHorizontal()) {
    // Inserting a horizontal run into a horizontal or vertical result. In both
    // cases, no adjustments are needed because |glyph_bounding_box_| is in
    // logical coordinates and uses alphabetic baseline.
    ComputeGlyphPositions<true>(run.get(), start_glyph, num_glyphs,
                                harf_buzz_buffer);
  } else {
    // Inserting a vertical run to a vertical result.
    ComputeGlyphPositions<false>(run.get(), start_glyph, num_glyphs,
                                 harf_buzz_buffer);
  }
  width_ += run->width_;
  num_glyphs_ += num_glyphs;
  DCHECK_GE(num_glyphs_, num_glyphs);

  InsertRun(std::move(run));
}

void ShapeResult::InsertRun(std::unique_ptr<ShapeResult::RunInfo> run) {
  // The runs are stored in result->m_runs in visual order. For LTR, we place
  // the run to be inserted before the next run with a bigger character
  // start index. For RTL, we place the run before the next run with a lower
  // character index. Otherwise, for both directions, at the end.
  if (HB_DIRECTION_IS_FORWARD(run->direction_)) {
    for (size_t pos = 0; pos < runs_.size(); ++pos) {
      if (runs_.at(pos)->start_index_ > run->start_index_) {
        runs_.insert(pos, std::move(run));
        break;
      }
    }
  } else {
    for (size_t pos = 0; pos < runs_.size(); ++pos) {
      if (runs_.at(pos)->start_index_ < run->start_index_) {
        runs_.insert(pos, std::move(run));
        break;
      }
    }
  }
  // If we didn't find an existing slot to place it, append.
  if (run)
    runs_.push_back(std::move(run));
}

// Insert a |RunInfo| without glyphs. |StartIndexForResult()| needs a run to
// compute the start character index. When all glyphs are missing, this function
// synthesize a run without glyphs.
void ShapeResult::InsertRunForIndex(unsigned start_character_index) {
  DCHECK(runs_.IsEmpty());
  runs_.push_back(std::make_unique<RunInfo>(
      primary_font_.get(), !Rtl() ? HB_DIRECTION_LTR : HB_DIRECTION_RTL,
      CanvasRotationInVertical::kRegular, HB_SCRIPT_UNKNOWN,
      start_character_index, 0, num_characters_));
}

ShapeResult::RunInfo* ShapeResult::InsertRunForTesting(
    unsigned start_index,
    unsigned num_characters,
    TextDirection direction,
    Vector<uint16_t> safe_break_offsets) {
  std::unique_ptr<RunInfo> run = std::make_unique<ShapeResult::RunInfo>(
      nullptr, IsLtr(direction) ? HB_DIRECTION_LTR : HB_DIRECTION_RTL,
      CanvasRotationInVertical::kRegular, HB_SCRIPT_COMMON, start_index, 0,
      num_characters);
  run->safe_break_offsets_.AppendVector(safe_break_offsets);
  RunInfo* run_ptr = run.get();
  InsertRun(std::move(run));
  return run_ptr;
}

// Moves runs at (run_size_before, end) to the front of |runs_|.
//
// Runs in RTL result are in visual order, and that new runs should be
// prepended. This function adjusts the run order after runs were appended.
void ShapeResult::ReorderRtlRuns(unsigned run_size_before) {
  DCHECK(Rtl());
  DCHECK_GT(runs_.size(), run_size_before);
  if (runs_.size() == run_size_before + 1) {
    if (!run_size_before)
      return;
    std::unique_ptr<RunInfo> new_run(std::move(runs_.back()));
    runs_.Shrink(runs_.size() - 1);
    runs_.push_front(std::move(new_run));
    return;
  }

  // |push_front| is O(n) that we should not call it multiple times.
  // Create a new list in the correct order and swap it.
  Vector<std::unique_ptr<RunInfo>> new_runs;
  new_runs.ReserveInitialCapacity(runs_.size());
  for (unsigned i = run_size_before; i < runs_.size(); i++)
    new_runs.push_back(std::move(runs_[i]));

  // Then append existing runs.
  for (unsigned i = 0; i < run_size_before; i++)
    new_runs.push_back(std::move(runs_[i]));
  runs_.swap(new_runs);
}

// Returns the left of the glyph bounding box of the left most character.
float ShapeResult::LineLeftBounds() const {
  DCHECK(!runs_.IsEmpty());
  const RunInfo& run = *runs_.front();
  const bool is_horizontal_run = run.IsHorizontal();
  const SimpleFontData& font_data = *run.font_data_;
  DCHECK(!run.glyph_data_.IsEmpty()) << ToString();
  const unsigned character_index = run.glyph_data_.front().character_index;
  GlyphBoundsAccumulator bounds(0.f);
  for (const auto& glyph : run.glyph_data_) {
    if (character_index != glyph.character_index)
      break;
    bounds.Unite(is_horizontal_run, glyph,
                 font_data.BoundsForGlyph(glyph.glyph));
    bounds.origin += glyph.advance;
  }
  if (UNLIKELY(!is_horizontal_run))
    bounds.ConvertVerticalRunToLogical(font_data.GetFontMetrics());
  return bounds.bounds.X();
}

// Returns the right of the glyph bounding box of the right most character.
float ShapeResult::LineRightBounds() const {
  DCHECK(!runs_.IsEmpty());
  const RunInfo& run = *runs_.back();
  const bool is_horizontal_run = run.IsHorizontal();
  const SimpleFontData& font_data = *run.font_data_;
  DCHECK(!run.glyph_data_.IsEmpty()) << ToString();
  const unsigned character_index = run.glyph_data_.back().character_index;
  GlyphBoundsAccumulator bounds(width_);
  for (auto glyph_it = run.glyph_data_.rbegin();
       glyph_it != run.glyph_data_.rend(); ++glyph_it) {
    const auto& glyph = *glyph_it;
    if (character_index != glyph.character_index)
      break;
    bounds.origin -= glyph.advance;
    bounds.Unite(is_horizontal_run, glyph,
                 font_data.BoundsForGlyph(glyph.glyph));
  }
  // If the last character has no ink (e.g., space character), assume the
  // character before will not overflow more than the width of the space.
  if (UNLIKELY(bounds.bounds.IsEmpty()))
    return width_;
  if (UNLIKELY(!is_horizontal_run))
    bounds.ConvertVerticalRunToLogical(font_data.GetFontMetrics());
  return bounds.bounds.MaxX();
}

void ShapeResult::CopyRange(unsigned start_offset,
                            unsigned end_offset,
                            ShapeResult* target) const {
  if (!runs_.size())
    return;

#if DCHECK_IS_ON()
  unsigned target_num_characters_before = target->num_characters_;
#endif

  // When |target| is empty, its character indexes are the specified sub range
  // of |this|. Otherwise the character indexes are renumbered to be continuous.
  int index_diff = !target->num_characters_
                       ? 0
                       : target->EndIndexForResult() -
                             std::max(start_offset, StartIndexForResult());
  unsigned target_run_size_before = target->runs_.size();
  float total_width = 0;
  for (const auto& run : runs_) {
    unsigned run_start = run->start_index_;
    unsigned run_end = run_start + run->num_characters_;

    if (start_offset < run_end && end_offset > run_start) {
      unsigned start = start_offset > run_start ? start_offset - run_start : 0;
      unsigned end = std::min(end_offset, run_end) - run_start;
      DCHECK(end > start);

      auto sub_run = run->CreateSubRun(start, end);
      sub_run->start_index_ += index_diff;
      total_width += sub_run->width_;
      target->num_characters_ += sub_run->num_characters_;
      target->num_glyphs_ += sub_run->glyph_data_.size();
      target->runs_.push_back(std::move(sub_run));
    }
  }

  if (!target->num_glyphs_)
    return;

  // Runs in RTL result are in visual order, and that new runs should be
  // prepended. Reorder appended runs.
  DCHECK_EQ(Rtl(), target->Rtl());
  if (UNLIKELY(Rtl() && target->runs_.size() != target_run_size_before))
    target->ReorderRtlRuns(target_run_size_before);

  // Compute new glyph bounding box.
  //
  // Computing glyph bounding box from Font is one of the most expensive
  // operations. If |start_offset| or |end_offset| are the start/end of |this|,
  // use the current |glyph_bounding_box_| for the side.
  DCHECK(primary_font_.get() == target->primary_font_.get());
  bool know_left_edge = start_offset <= StartIndexForResult();
  bool know_right_edge = end_offset >= EndIndexForResult();
  if (UNLIKELY(Rtl()))
    std::swap(know_left_edge, know_right_edge);
  float left = know_left_edge ? target->width_ + glyph_bounding_box_.X()
                              : target->LineLeftBounds();
  target->width_ += total_width;
  float right = know_right_edge
                    ? glyph_bounding_box_.MaxX() - width_ + target->width_
                    : target->LineRightBounds();
  FloatRect adjusted_box(left, glyph_bounding_box_.Y(),
                         std::max(right - left, 0.0f),
                         glyph_bounding_box_.Height());
  target->glyph_bounding_box_.UniteIfNonZero(adjusted_box);

  target->has_vertical_offsets_ |= has_vertical_offsets_;

#if DCHECK_IS_ON()
  DCHECK_EQ(target->num_characters_ - target_num_characters_before,
            std::min(end_offset, EndIndexForResult()) -
                std::max(start_offset, StartIndexForResult()));

  target->CheckConsistency();
#endif
}

scoped_refptr<ShapeResult> ShapeResult::SubRange(unsigned start_offset,
                                                 unsigned end_offset) const {
  scoped_refptr<ShapeResult> sub_range =
      Create(primary_font_.get(), 0, Direction());
  CopyRange(start_offset, end_offset, sub_range.get());
  return sub_range;
}

scoped_refptr<ShapeResult> ShapeResult::CopyAdjustedOffset(
    unsigned start_index) const {
  scoped_refptr<ShapeResult> result = base::AdoptRef(new ShapeResult(*this));

  if (start_index > result->StartIndexForResult()) {
    unsigned delta = start_index - result->StartIndexForResult();
    for (auto& run : result->runs_)
      run->start_index_ += delta;
  } else {
    unsigned delta = result->StartIndexForResult() - start_index;
    for (auto& run : result->runs_) {
      DCHECK(run->start_index_ >= delta);
      run->start_index_ -= delta;
    }
  }

  return result;
}

#if DCHECK_IS_ON()
void ShapeResult::CheckConsistency() const {
  if (runs_.IsEmpty()) {
    DCHECK_EQ(0u, num_characters_);
    DCHECK_EQ(0u, num_glyphs_);
    return;
  }

  unsigned index = StartIndexForResult();
  unsigned num_glyphs = 0;
  if (!Rtl()) {
    for (const auto& run : runs_) {
      DCHECK_EQ(index, run->start_index_);
      index += run->num_characters_;
      num_glyphs += run->glyph_data_.size();
    }
  } else {
    // RTL on Mac may not have runs for the all characters. crbug.com/774034
    index = runs_.back()->start_index_;
    for (auto it = runs_.rbegin(); it != runs_.rend(); ++it) {
      const auto& run = *it;
      DCHECK_EQ(index, run->start_index_);
      index += run->num_characters_;
      num_glyphs += run->glyph_data_.size();
    }
  }
  DCHECK_EQ(index, EndIndexForResult());
  DCHECK_EQ(num_glyphs, num_glyphs_);
}
#endif

scoped_refptr<ShapeResult> ShapeResult::CreateForTabulationCharacters(
    const Font* font,
    const TextRun& text_run,
    float position_offset,
    unsigned count) {
  const SimpleFontData* font_data = font->PrimaryFont();
  // Tab characters are always LTR or RTL, not TTB, even when
  // isVerticalAnyUpright().
  std::unique_ptr<ShapeResult::RunInfo> run = std::make_unique<RunInfo>(
      font_data, text_run.Rtl() ? HB_DIRECTION_RTL : HB_DIRECTION_LTR,
      CanvasRotationInVertical::kRegular, HB_SCRIPT_COMMON, 0, count, count);
  float position = text_run.XPos() + position_offset;
  float start_position = position;
  for (unsigned i = 0; i < count; i++) {
    float advance = font->TabWidth(font_data, text_run.GetTabSize(), position);
    HarfBuzzRunGlyphData& glyph_data = run->glyph_data_[i];
    glyph_data.SetGlyphAndPositions(font_data->SpaceGlyph(), i, advance,
                                    FloatSize());

    // Assume it's safe to break after a tab character.
    run->safe_break_offsets_.push_back(glyph_data.character_index);
    position += advance;
  }
  run->width_ = position - start_position;

  scoped_refptr<ShapeResult> result =
      ShapeResult::Create(font, count, text_run.Direction());
  result->width_ = run->width_;
  result->num_glyphs_ = count;
  DCHECK_EQ(result->num_glyphs_, count);  // no overflow
  result->has_vertical_offsets_ =
      font_data->PlatformData().IsVerticalAnyUpright();
  result->runs_.push_back(std::move(run));
  return result;
}

void ShapeResult::ToString(StringBuilder* output) const {
  output->Append("#chars=");
  output->AppendNumber(num_characters_);
  output->Append(", #glyphs=");
  output->AppendNumber(num_glyphs_);
  output->Append(", dir=");
  output->AppendNumber(direction_);
  output->Append(", runs[");
  output->AppendNumber(runs_.size());
  output->Append("]{");
  for (unsigned run_index = 0; run_index < runs_.size(); run_index++) {
    output->AppendNumber(run_index);
    const auto& run = *runs_[run_index];
    output->Append(":{start=");
    output->AppendNumber(run.start_index_);
    output->Append(", #chars=");
    output->AppendNumber(run.num_characters_);
    output->Append(", dir=");
    output->AppendNumber(run.direction_);
    output->Append(", glyphs[");
    output->AppendNumber(run.glyph_data_.size());
    output->Append("]{");
    for (unsigned glyph_index = 0; glyph_index < run.glyph_data_.size();
         glyph_index++) {
      output->AppendNumber(glyph_index);
      const auto& glyph_data = run.glyph_data_[glyph_index];
      output->Append(":{char=");
      output->AppendNumber(glyph_data.character_index);
      output->Append(", glyph=");
      output->AppendNumber(glyph_data.glyph);
      output->Append("}");
    }
    output->Append("}}");
  }
  output->Append("}");
}

String ShapeResult::ToString() const {
  StringBuilder output;
  ToString(&output);
  return output.ToString();
}

}  // namespace blink
