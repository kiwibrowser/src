// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_RENDER_TEXT_HARFBUZZ_H_
#define UI_GFX_RENDER_TEXT_HARFBUZZ_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "third_party/icu/source/common/unicode/ubidi.h"
#include "third_party/icu/source/common/unicode/uscript.h"
#include "ui/gfx/render_text.h"

#include <hb.h>

namespace base {
namespace i18n {
class BreakIterator;
}
}

namespace gfx {

class Range;
class RangeF;
class RenderTextHarfBuzz;

namespace internal {

struct GFX_EXPORT TextRunHarfBuzz {
  // Construct the run with |template_font| since determining the details of a
  // default-constructed gfx::Font is expensive, but it will always be replaced.
  explicit TextRunHarfBuzz(const Font& template_font);
  ~TextRunHarfBuzz();

  // Returns the index of the first glyph that corresponds to the character at
  // |pos|.
  size_t CharToGlyph(size_t pos) const;

  // Returns the corresponding glyph range of the given character range.
  // |range| is in text-space (0 corresponds to |GetDisplayText()[0]|). Returned
  // value is in run-space (0 corresponds to the first glyph in the run).
  Range CharRangeToGlyphRange(const Range& range) const;

  // Returns the number of missing glyphs in the shaped text run.
  size_t CountMissingGlyphs() const;

  // Writes the character and glyph ranges of the cluster containing |pos|.
  void GetClusterAt(size_t pos, Range* chars, Range* glyphs) const;

  // Returns the grapheme bounds at |text_index|. Handles multi-grapheme glyphs.
  // Returned value is the horizontal pixel span in text-space (assumes all runs
  // are on the same line). The returned range is never reversed.
  RangeF GetGraphemeBounds(RenderTextHarfBuzz* render_text,
                           size_t text_index) const;

  // Returns the horizontal span of the given |char_range| handling grapheme
  // boundaries within glyphs. This is a wrapper around one or more calls to
  // GetGraphemeBounds(), returning a range in the same coordinate space.
  RangeF GetGraphemeSpanForCharRange(RenderTextHarfBuzz* render_text,
                                     const Range& char_range) const;

  // Returns the glyph width for the given character range. |char_range| is in
  // text-space (0 corresponds to |GetDisplayText()[0]|).
  SkScalar GetGlyphWidthForCharRange(const Range& char_range) const;

  float width;
  float preceding_run_widths;
  Range range;
  bool is_rtl;
  UBiDiLevel level;
  UScriptCode script;

  std::unique_ptr<uint16_t[]> glyphs;
  std::unique_ptr<SkPoint[]> positions;
  std::vector<uint32_t> glyph_to_char;
  size_t glyph_count;

  Font font;
  sk_sp<SkTypeface> skia_face;
  FontRenderParams render_params;
  int font_size;
  int baseline_offset;
  int baseline_type;
  bool italic;
  Font::Weight weight;
  bool strike;
  bool underline;
  bool heavy_underline;

 private:
  DISALLOW_COPY_AND_ASSIGN(TextRunHarfBuzz);
};

// Manages the list of TextRunHarfBuzz and its logical <-> visual index mapping.
class TextRunList {
 public:
  TextRunList();
  ~TextRunList();

  size_t size() const { return runs_.size(); }

  // Converts the index between logical and visual index.
  size_t visual_to_logical(size_t index) const {
    return visual_to_logical_[index];
  }
  size_t logical_to_visual(size_t index) const {
    return logical_to_visual_[index];
  }

  const std::vector<std::unique_ptr<TextRunHarfBuzz>>& runs() const {
    return runs_;
  }

  // Adds the new |run| to the run list.
  void Add(std::unique_ptr<TextRunHarfBuzz> run) {
    runs_.push_back(std::move(run));
  }

  // Reset the run list.
  void Reset();

  // Initialize the index mapping.
  void InitIndexMap();

  // Precomputes the offsets for all runs.
  void ComputePrecedingRunWidths();

  // Get the total width of runs, as if they were shown on one line.
  // Do not use this when multiline is enabled.
  float width() const { return width_; }

  // Get the run index applicable to |position| (at or preceeding |position|).
  size_t GetRunIndexAt(size_t position) const;

 private:
  // Text runs in logical order.
  std::vector<std::unique_ptr<TextRunHarfBuzz>> runs_;

  // Maps visual run indices to logical run indices and vice versa.
  std::vector<int32_t> visual_to_logical_;
  std::vector<int32_t> logical_to_visual_;

  float width_;

  DISALLOW_COPY_AND_ASSIGN(TextRunList);
};

}  // namespace internal

class GFX_EXPORT RenderTextHarfBuzz : public RenderText {
 public:
  RenderTextHarfBuzz();
  ~RenderTextHarfBuzz() override;

  // RenderText:
  std::unique_ptr<RenderText> CreateInstanceOfSameType() const override;
  bool MultilineSupported() const override;
  const base::string16& GetDisplayText() override;
  Size GetStringSize() override;
  SizeF GetStringSizeF() override;
  SelectionModel FindCursorPosition(const Point& point) override;
  bool IsSelectionSupported() const override;
  std::vector<FontSpan> GetFontSpansForTesting() override;
  Range GetCursorSpan(const Range& text_range) override;

  // ICU grapheme iterator for the layout text. Can be null in case of an error.
  base::i18n::BreakIterator* GetGraphemeIterator();

 protected:
  // RenderText:
  int GetDisplayTextBaseline() override;
  SelectionModel AdjacentCharSelectionModel(
      const SelectionModel& selection,
      VisualCursorDirection direction) override;
  SelectionModel AdjacentWordSelectionModel(
      const SelectionModel& selection,
      VisualCursorDirection direction) override;
  std::vector<Rect> GetSubstringBounds(const Range& range) override;
  size_t TextIndexToDisplayIndex(size_t index) override;
  size_t DisplayIndexToTextIndex(size_t index) override;
  bool IsValidCursorIndex(size_t index) override;
  void OnLayoutTextAttributeChanged(bool text_changed) override;
  void OnDisplayTextAttributeChanged() override;
  void EnsureLayout() override;
  void DrawVisualText(internal::SkiaTextRenderer* renderer) override;

 private:
  friend class test::RenderTextTestApi;
  friend class RenderTextHarfBuzzTest;

  // Return the run index that contains the argument; or the length of the
  // |runs_| vector if argument exceeds the text length or width.
  size_t GetRunContainingCaret(const SelectionModel& caret);

  // Given a |run|, returns the SelectionModel that contains the logical first
  // or last caret position inside (not at a boundary of) the run.
  // The returned value represents a cursor/caret position without a selection.
  SelectionModel FirstSelectionModelInsideRun(
      const internal::TextRunHarfBuzz* run);
  SelectionModel LastSelectionModelInsideRun(
      const internal::TextRunHarfBuzz* run);

  // Break the text into logical runs and populate the visual <-> logical maps
  // into |run_list_out|.
  void ItemizeTextToRuns(const base::string16& string,
                         internal::TextRunList* run_list_out);

  // Helper method for ShapeRun() that calls ShapeRunWithFont() with |text|,
  // |run|, |font|, and |render_params|, returning true if the font provides
  // all the glyphs needed for |run|, and false otherwise. Additionally updates
  // |best_font|, |best_render_params|, and |best_missing_glyphs| if |font|
  // has fewer than |best_missing_glyphs| missing glyphs.
  bool CompareFamily(const base::string16& text,
                     const Font& font,
                     const FontRenderParams& render_params,
                     internal::TextRunHarfBuzz* run,
                     Font* best_font,
                     FontRenderParams* best_render_params,
                     size_t* best_missing_glyphs);

  // Shape the glyphs of all runs in |run_list| using |text|.
  void ShapeRunList(const base::string16& text,
                    internal::TextRunList* run_list);

  // Shape the glyphs needed for the |run| within the |text|.
  void ShapeRun(const base::string16& text,
                internal::TextRunHarfBuzz* run);
  bool ShapeRunWithFont(const base::string16& text,
                        const Font& font,
                        const FontRenderParams& params,
                        internal::TextRunHarfBuzz* run);

  // Makes sure that text runs for layout text are shaped.
  void EnsureLayoutRunList();

  // RenderText:
  internal::TextRunList* GetRunList() override;
  const internal::TextRunList* GetRunList() const override;
  bool GetDecoratedTextForRange(const Range& range,
                                DecoratedText* decorated_text) override;
  void SetGlyphWidthForTest(float test_width) override;

  // Text run list for |layout_text_| and |display_text_|.
  // |display_run_list_| is created only when the text is elided.
  internal::TextRunList layout_run_list_;
  std::unique_ptr<internal::TextRunList> display_run_list_;

  bool update_layout_run_list_ : 1;
  bool update_display_run_list_ : 1;
  bool update_grapheme_iterator_ : 1;
  bool update_display_text_ : 1;

  // ICU grapheme iterator for the layout text. Use GetGraphemeIterator()
  // to access the iterator.
  std::unique_ptr<base::i18n::BreakIterator> grapheme_iterator_;

  // The total size of the layouted text.
  SizeF total_size_;

  // Fixed width of glyphs. This should only be set in test environments.
  float glyph_width_for_test_;

  DISALLOW_COPY_AND_ASSIGN(RenderTextHarfBuzz);
};

}  // namespace gfx

#endif  // UI_GFX_RENDER_TEXT_HARFBUZZ_H_
