// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_SHAPING_SHAPE_RESULT_BUFFER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_SHAPING_SHAPE_RESULT_BUFFER_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

struct CharacterRange;
class FontDescription;
struct GlyphData;
class ShapeResultBloberizer;
class TextRun;

class PLATFORM_EXPORT ShapeResultBuffer {
  WTF_MAKE_NONCOPYABLE(ShapeResultBuffer);
  STACK_ALLOCATED();

 public:
  ShapeResultBuffer() : has_vertical_offsets_(false) {}

  void AppendResult(scoped_refptr<const ShapeResult> result) {
    has_vertical_offsets_ |= result->HasVerticalOffsets();
    results_.push_back(std::move(result));
  }

  bool HasVerticalOffsets() const { return has_vertical_offsets_; }

  int OffsetForPosition(const TextRun&,
                        float target_x,
                        bool include_partial_glyphs) const;
  CharacterRange GetCharacterRange(TextDirection,
                                   float total_width,
                                   unsigned from,
                                   unsigned to) const;
  Vector<CharacterRange> IndividualCharacterRanges(TextDirection,
                                                   float total_width) const;

  static CharacterRange GetCharacterRange(scoped_refptr<const ShapeResult>,
                                          TextDirection,
                                          float total_width,
                                          unsigned from,
                                          unsigned to);

  Vector<ShapeResult::RunFontData> GetRunFontData() const;

  GlyphData EmphasisMarkGlyphData(const FontDescription&) const;

 private:
  friend class ShapeResultBloberizer;
  static CharacterRange GetCharacterRangeInternal(
      const Vector<scoped_refptr<const ShapeResult>, 64>&,
      TextDirection,
      float total_width,
      unsigned from,
      unsigned to);

  static void AddRunInfoRanges(const ShapeResult::RunInfo&,
                               float offset,
                               Vector<CharacterRange>&);

  // Empirically, cases where we get more than 50 ShapeResults are extremely
  // rare.
  Vector<scoped_refptr<const ShapeResult>, 64> results_;
  bool has_vertical_offsets_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_SHAPING_SHAPE_RESULT_BUFFER_H_
