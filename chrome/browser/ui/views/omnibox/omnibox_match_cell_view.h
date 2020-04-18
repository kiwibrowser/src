// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_MATCH_CELL_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_MATCH_CELL_VIEW_H_

#include "ui/views/view.h"

namespace views {
class ImageView;
}

struct AutocompleteMatch;
class OmniboxResultView;
class OmniboxTextView;

class OmniboxMatchCellView : public views::View {
 public:
  explicit OmniboxMatchCellView(OmniboxResultView* result_view);
  ~OmniboxMatchCellView() override;

  views::ImageView* icon() { return icon_view_; }
  views::ImageView* image() { return image_view_; }
  OmniboxTextView* content() { return content_view_; }
  OmniboxTextView* description() { return description_view_; }
  OmniboxTextView* separator() { return separator_view_; }

  void OnMatchUpdate(const OmniboxResultView* result_view,
                     const AutocompleteMatch& match);

  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  bool CanProcessEventsWithinSubtree() const override;

 protected:
  // views::View:
  void Layout() override;
  const char* GetClassName() const override;

  // Returns the height of the the description section of answer suggestions.
  int GetDescriptionHeight() const;

  void LayoutOldStyleAnswer();
  void LayoutRichSuggestion();
  void LayoutSplit();

  bool is_old_style_answer_;
  bool is_rich_suggestion_;
  bool is_search_type_;

  // Weak pointers for easy reference.
  views::ImageView* icon_view_;   // An icon representing the type or content.
  views::ImageView* image_view_;  // For rich suggestions.
  OmniboxTextView* content_view_;
  OmniboxTextView* description_view_;
  OmniboxTextView* separator_view_;

 private:
  DISALLOW_COPY_AND_ASSIGN(OmniboxMatchCellView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_MATCH_CELL_VIEW_H_
