// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/shaping/shape_result_buffer.h"

#include "third_party/blink/renderer/platform/fonts/character_range.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result_inline_headers.h"
#include "third_party/blink/renderer/platform/fonts/simple_font_data.h"
#include "third_party/blink/renderer/platform/geometry/float_point.h"
#include "third_party/blink/renderer/platform/text/text_direction.h"

namespace blink {

// TODO(eae): This is a bit of a hack to allow reuse of the implementation
// for both ShapeResultBuffer and single ShapeResult use cases. Ideally the
// logic should move into ShapeResult itself and then the ShapeResultBuffer
// implementation may wrap that.
CharacterRange ShapeResultBuffer::GetCharacterRange(
    scoped_refptr<const ShapeResult> result,
    TextDirection direction,
    float total_width,
    unsigned from,
    unsigned to) {
  Vector<scoped_refptr<const ShapeResult>, 64> results;
  results.push_back(result);
  return GetCharacterRangeInternal(results, direction, total_width, from, to);
}

CharacterRange ShapeResultBuffer::GetCharacterRangeInternal(
    const Vector<scoped_refptr<const ShapeResult>, 64>& results,
    TextDirection direction,
    float total_width,
    unsigned absolute_from,
    unsigned absolute_to) {
  float current_x = 0;
  float from_x = 0;
  float to_x = 0;
  bool found_from_x = false;
  bool found_to_x = false;
  float min_y = 0;
  float max_y = 0;

  if (direction == TextDirection::kRtl)
    current_x = total_width;

  // The absoluteFrom and absoluteTo arguments represent the start/end offset
  // for the entire run, from/to are continuously updated to be relative to
  // the current word (ShapeResult instance).
  int from = absolute_from;
  int to = absolute_to;

  unsigned total_num_characters = 0;
  for (unsigned j = 0; j < results.size(); j++) {
    const scoped_refptr<const ShapeResult> result = results[j];
    if (direction == TextDirection::kRtl) {
      // Convert logical offsets to visual offsets, because results are in
      // logical order while runs are in visual order.
      if (!found_from_x && from >= 0 &&
          static_cast<unsigned>(from) < result->NumCharacters())
        from = result->NumCharacters() - from - 1;
      if (!found_to_x && to >= 0 &&
          static_cast<unsigned>(to) < result->NumCharacters())
        to = result->NumCharacters() - to - 1;
      current_x -= result->Width();
    }
    for (unsigned i = 0; i < result->runs_.size(); i++) {
      if (!result->runs_[i])
        continue;
      DCHECK_EQ(direction == TextDirection::kRtl, result->runs_[i]->Rtl());
      int num_characters = result->runs_[i]->num_characters_;
      if (!found_from_x && from >= 0 && from < num_characters) {
        from_x = result->runs_[i]->XPositionForVisualOffset(
                     from, AdjustMidCluster::kToStart) +
                 current_x;
        found_from_x = true;
      } else {
        from -= num_characters;
      }

      if (!found_to_x && to >= 0 && to < num_characters) {
        to_x = result->runs_[i]->XPositionForVisualOffset(
                   to, AdjustMidCluster::kToEnd) +
               current_x;
        found_to_x = true;
      } else {
        to -= num_characters;
      }

      if (found_from_x || found_to_x) {
        min_y = std::min(min_y, result->Bounds().Y());
        max_y = std::max(max_y, result->Bounds().MaxY());
      }

      if (found_from_x && found_to_x)
        break;
      current_x += result->runs_[i]->width_;
    }
    if (direction == TextDirection::kRtl)
      current_x -= result->Width();
    total_num_characters += result->NumCharacters();
  }

  // The position in question might be just after the text.
  if (!found_from_x && absolute_from == total_num_characters) {
    from_x = direction == TextDirection::kRtl ? 0 : total_width;
    found_from_x = true;
  }
  if (!found_to_x && absolute_to == total_num_characters) {
    to_x = direction == TextDirection::kRtl ? 0 : total_width;
    found_to_x = true;
  }
  if (!found_from_x)
    from_x = 0;
  if (!found_to_x)
    to_x = direction == TextDirection::kRtl ? 0 : total_width;

  // None of our runs is part of the selection, possibly invalid arguments.
  if (!found_to_x && !found_from_x)
    from_x = to_x = 0;
  if (from_x < to_x)
    return CharacterRange(from_x, to_x, -min_y, max_y);
  return CharacterRange(to_x, from_x, -min_y, max_y);
}

CharacterRange ShapeResultBuffer::GetCharacterRange(TextDirection direction,
                                                    float total_width,
                                                    unsigned from,
                                                    unsigned to) const {
  return GetCharacterRangeInternal(results_, direction, total_width, from, to);
}

void ShapeResultBuffer::AddRunInfoRanges(const ShapeResult::RunInfo& run_info,
                                         float offset,
                                         Vector<CharacterRange>& ranges) {
  Vector<float> character_widths(run_info.num_characters_);
  for (const auto& glyph : run_info.glyph_data_)
    character_widths[glyph.character_index] += glyph.advance;

  for (unsigned character_index = 0; character_index < run_info.num_characters_;
       character_index++) {
    float start = offset;
    offset += character_widths[character_index];
    float end = offset;

    // To match getCharacterRange we flip ranges to ensure start <= end.
    if (end < start)
      ranges.push_back(CharacterRange(end, start, 0, 0));
    else
      ranges.push_back(CharacterRange(start, end, 0, 0));
  }
}

Vector<CharacterRange> ShapeResultBuffer::IndividualCharacterRanges(
    TextDirection direction,
    float total_width) const {
  Vector<CharacterRange> ranges;
  float current_x = direction == TextDirection::kRtl ? total_width : 0;
  for (const scoped_refptr<const ShapeResult> result : results_) {
    if (direction == TextDirection::kRtl)
      current_x -= result->Width();
    unsigned run_count = result->runs_.size();
    for (unsigned index = 0; index < run_count; index++) {
      unsigned run_index =
          direction == TextDirection::kRtl ? run_count - 1 - index : index;
      AddRunInfoRanges(*result->runs_[run_index], current_x, ranges);
      current_x += result->runs_[run_index]->width_;
    }
    if (direction == TextDirection::kRtl)
      current_x -= result->Width();
  }
  return ranges;
}

int ShapeResultBuffer::OffsetForPosition(const TextRun& run,
                                         float target_x,
                                         bool include_partial_glyphs) const {
  unsigned total_offset;
  if (run.Rtl()) {
    total_offset = run.length();
    for (unsigned i = results_.size(); i; --i) {
      const scoped_refptr<const ShapeResult>& word_result = results_[i - 1];
      if (!word_result)
        continue;
      total_offset -= word_result->NumCharacters();
      if (target_x >= 0 && target_x <= word_result->Width()) {
        int offset_for_word =
            word_result->OffsetForPosition(target_x, include_partial_glyphs);
        return total_offset + offset_for_word;
      }
      target_x -= word_result->Width();
    }
  } else {
    total_offset = 0;
    for (const auto& word_result : results_) {
      if (!word_result)
        continue;
      int offset_for_word =
          word_result->OffsetForPosition(target_x, include_partial_glyphs);
      DCHECK_GE(offset_for_word, 0);
      total_offset += offset_for_word;
      if (target_x >= 0 && target_x <= word_result->Width())
        return total_offset;
      target_x -= word_result->Width();
    }
  }
  return total_offset;
}

Vector<ShapeResult::RunFontData> ShapeResultBuffer::GetRunFontData() const {
  Vector<ShapeResult::RunFontData> font_data;
  for (const auto& result : results_)
    result->GetRunFontData(&font_data);
  return font_data;
}

GlyphData ShapeResultBuffer::EmphasisMarkGlyphData(
    const FontDescription& font_description) const {
  for (const auto& result : results_) {
    for (const auto& run : result->runs_) {
      DCHECK(run->font_data_);
      if (run->glyph_data_.IsEmpty())
        continue;

      return GlyphData(
          run->glyph_data_[0].glyph,
          run->font_data_->EmphasisMarkFontData(font_description).get(),
          run->CanvasRotation());
    }
  }

  return GlyphData();
}

}  // namespace blink
