// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_TEXT_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_TEXT_VIEW_H_

#include <stddef.h>

#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/views/omnibox/omnibox_result_view.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/suggestion_answer.h"
#include "ui/gfx/font_list.h"
#include "ui/views/view.h"

namespace gfx {
class Canvas;
class RenderText;
}  // namespace gfx

// A view containing a render text styled via search results. This differs from
// the general purpose views::Label class by having less general features (such
// as selection) and more specific features (such as suggestion answer styling).
class OmniboxTextView : public views::View {
 public:
  explicit OmniboxTextView(OmniboxResultView* result_view);
  ~OmniboxTextView() override;

  // views::View.
  gfx::Size CalculatePreferredSize() const override;
  bool CanProcessEventsWithinSubtree() const override;
  const char* GetClassName() const override;
  int GetHeightForWidth(int width) const override;
  void OnPaint(gfx::Canvas* canvas) override;

  // Dim the text (i.e. make it gray). This is used for secondary text (so that
  // the non-dimmed text stands out more).
  void Dim();

  // Creates a RenderText with default rendering for the given |text|. The
  // |classifications| are used to style the text. An ImageLine incorporates
  // both the text and the styling.
  void SetText(const base::string16& text,
               const ACMatchClassifications& classifications);
  void SetText(const SuggestionAnswer::ImageLine& line);
  void SetText(const base::string16& text);

  // Get the height of one line of text.  This is handy if the view might have
  // multiple lines.
  int GetLineHeight() const;

 private:
  std::unique_ptr<gfx::RenderText> CreateRenderText(
      const base::string16& text) const;

  // Similar to CreateRenderText, but also apply styling (classifications).
  std::unique_ptr<gfx::RenderText> CreateClassifiedRenderText(
      const base::string16& text,
      const ACMatchClassifications& classifications) const;

  // Creates a RenderText with text and styling from the image line.
  std::unique_ptr<gfx::RenderText> CreateText(
      const SuggestionAnswer::ImageLine& line) const;

  // Adds |text| to |destination|.  |text_type| is an index into the
  // kTextStyles constant defined in the .cc file and is used to style the text,
  // including setting the font size, color, and baseline style.  See the
  // TextStyle struct in the .cc file for more.
  void AppendText(gfx::RenderText* destination,
                  const base::string16& text,
                  int text_type) const;

  // AppendText will break up the |text| into bold and non-bold pieces
  // and pass each to this helper with the correct |is_bold| value.
  void AppendTextHelper(gfx::RenderText* destination,
                        const base::string16& text,
                        int text_type,
                        bool is_bold) const;

  void UpdateLineHeight();

  // To get color values.
  OmniboxResultView* result_view_;

  // Font settings for this view.
  int font_height_;

  // Whether to wrap lines if the width is too narrow for the whole string.
  bool wrap_text_lines_;

  // The primary data for this class.
  mutable std::unique_ptr<gfx::RenderText> render_text_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxTextView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_TEXT_VIEW_H_
