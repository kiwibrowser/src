/*
 * Copyright (C) 2015 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
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

#include "third_party/blink/renderer/platform/fonts/shaping/caching_word_shaper.h"

#include "third_party/blink/renderer/platform/fonts/character_range.h"
#include "third_party/blink/renderer/platform/fonts/shaping/caching_word_shape_iterator.h"
#include "third_party/blink/renderer/platform/fonts/shaping/harf_buzz_shaper.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_cache.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result_buffer.h"
#include "third_party/blink/renderer/platform/fonts/simple_font_data.h"
#include "third_party/blink/renderer/platform/fonts/text_run_paint_info.h"
#include "third_party/blink/renderer/platform/wtf/text/character_names.h"

namespace blink {

ShapeCache* CachingWordShaper::GetShapeCache() const {
  return font_.font_fallback_list_->GetShapeCache(font_.font_description_);
}

float CachingWordShaper::Width(const TextRun& run,
                               HashSet<const SimpleFontData*>* fallback_fonts,
                               FloatRect* glyph_bounds) {
  float width = 0;
  scoped_refptr<const ShapeResult> word_result;
  CachingWordShapeIterator iterator(GetShapeCache(), run, &font_);
  while (iterator.Next(&word_result)) {
    if (word_result) {
      if (glyph_bounds) {
        FloatRect adjusted_bounds = word_result->Bounds();
        // Translate glyph bounds to the current glyph position which
        // is the total width before this glyph.
        adjusted_bounds.SetX(adjusted_bounds.X() + width);
        glyph_bounds->Unite(adjusted_bounds);
      }
      width += word_result->Width();
      if (fallback_fonts)
        word_result->FallbackFonts(fallback_fonts);
    }
  }

  return width;
}

static inline float ShapeResultsForRun(ShapeCache* shape_cache,
                                       const Font* font,
                                       const TextRun& run,
                                       ShapeResultBuffer* results_buffer) {
  CachingWordShapeIterator iterator(shape_cache, run, font);
  scoped_refptr<const ShapeResult> word_result;
  float total_width = 0;
  while (iterator.Next(&word_result)) {
    if (word_result) {
      total_width += word_result->Width();
      results_buffer->AppendResult(std::move(word_result));
    }
  }
  return total_width;
}

int CachingWordShaper::OffsetForPosition(const TextRun& run,
                                         float target_x,
                                         bool include_partial_glyphs) {
  ShapeResultBuffer buffer;
  ShapeResultsForRun(GetShapeCache(), &font_, run, &buffer);

  return buffer.OffsetForPosition(run, target_x, include_partial_glyphs);
}

void CachingWordShaper::FillResultBuffer(const TextRunPaintInfo& run_info,
                                         ShapeResultBuffer* buffer) {
  DCHECK(buffer);
  ShapeResultsForRun(GetShapeCache(), &font_, run_info.run, buffer);
}

CharacterRange CachingWordShaper::GetCharacterRange(const TextRun& run,
                                                    unsigned from,
                                                    unsigned to) {
  ShapeResultBuffer buffer;
  float total_width = ShapeResultsForRun(GetShapeCache(), &font_, run, &buffer);

  return buffer.GetCharacterRange(run.Direction(), total_width, from, to);
}

Vector<CharacterRange> CachingWordShaper::IndividualCharacterRanges(
    const TextRun& run) {
  ShapeResultBuffer buffer;
  float total_width = ShapeResultsForRun(GetShapeCache(), &font_, run, &buffer);

  auto ranges = buffer.IndividualCharacterRanges(run.Direction(), total_width);
  // The shaper can fail to return glyph metrics for all characters (see
  // crbug.com/613915 and crbug.com/615661) so add empty ranges to ensure all
  // characters have an associated range.
  while (ranges.size() < static_cast<unsigned>(run.length()))
    ranges.push_back(CharacterRange(0, 0, 0, 0));
  return ranges;
}

Vector<ShapeResult::RunFontData> CachingWordShaper::GetRunFontData(
    const TextRun& run) const {
  ShapeResultBuffer buffer;
  ShapeResultsForRun(GetShapeCache(), &font_, run, &buffer);

  return buffer.GetRunFontData();
}

GlyphData CachingWordShaper::EmphasisMarkGlyphData(
    const TextRun& emphasis_mark_run) const {
  ShapeResultBuffer buffer;
  ShapeResultsForRun(GetShapeCache(), &font_, emphasis_mark_run, &buffer);

  return buffer.EmphasisMarkGlyphData(font_.font_description_);
}

};  // namespace blink
