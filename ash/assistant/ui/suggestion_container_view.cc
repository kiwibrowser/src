// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/assistant/ui/suggestion_container_view.h"

#include <memory>

#include "ash/assistant/assistant_controller.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/views/controls/scrollbar/overlay_scroll_bar.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

namespace {

// TODO(dmblack): Move common dimensions to shared constant file.
// Appearance.
constexpr int kPaddingDip = 14;
constexpr int kPreferredHeightDip = 48;
constexpr int kSpacingDip = 8;

// InvisibleScrollBar ----------------------------------------------------------

class InvisibleScrollBar : public views::OverlayScrollBar {
 public:
  explicit InvisibleScrollBar(bool horizontal)
      : views::OverlayScrollBar(horizontal) {}

  ~InvisibleScrollBar() override = default;

  // views::OverlayScrollBar:
  int GetThickness() const override { return 0; }

 private:
  DISALLOW_COPY_AND_ASSIGN(InvisibleScrollBar);
};

}  // namespace

// SuggestionContainerView -----------------------------------------------------

SuggestionContainerView::SuggestionContainerView(
    AssistantController* assistant_controller)
    : assistant_controller_(assistant_controller),
      contents_view_(new views::View()),
      download_request_weak_factory_(this) {
  InitLayout();

  // The Assistant controller indirectly owns the view hierarchy to which
  // SuggestionContainerView belongs so is guaranteed to outlive it.
  assistant_controller_->AddInteractionModelObserver(this);
}

SuggestionContainerView::~SuggestionContainerView() {
  assistant_controller_->RemoveInteractionModelObserver(this);
}

gfx::Size SuggestionContainerView::CalculatePreferredSize() const {
  return gfx::Size(INT_MAX, GetHeightForWidth(INT_MAX));
}

int SuggestionContainerView::GetHeightForWidth(int width) const {
  return kPreferredHeightDip;
}

void SuggestionContainerView::InitLayout() {
  // Contents.
  views::BoxLayout* layout_manager =
      contents_view_->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(0, kPaddingDip), kSpacingDip));

  layout_manager->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::CROSS_AXIS_ALIGNMENT_CENTER);

  // ScrollView.
  SetBackgroundColor(SK_ColorTRANSPARENT);
  SetContents(contents_view_);
  SetHorizontalScrollBar(new InvisibleScrollBar(/*horizontal=*/true));
  SetVerticalScrollBar(new InvisibleScrollBar(/*horizontal=*/false));
}

void SuggestionContainerView::OnSuggestionsAdded(
    const std::map<int, AssistantSuggestion*>& suggestions) {
  for (const std::pair<int, AssistantSuggestion*>& suggestion : suggestions) {
    // We will use the same identifier by which the Assistant interaction model
    // uniquely identifies a suggestion to uniquely identify its corresponding
    // suggestion chip view.
    const int id = suggestion.first;

    app_list::SuggestionChipView::Params params;
    params.text = base::UTF8ToUTF16(suggestion.second->text);

    if (!suggestion.second->icon_url.is_empty()) {
      // Initiate a request to download the image for the suggestion chip icon.
      // Note that the request is identified by the suggestion id.
      assistant_controller_->DownloadImage(
          suggestion.second->icon_url,
          base::BindOnce(
              &SuggestionContainerView::OnSuggestionChipIconDownloaded,
              download_request_weak_factory_.GetWeakPtr(), id));

      // To reserve layout space until the actual icon can be downloaded, we
      // supply an empty placeholder image to the suggestion chip view.
      params.icon = gfx::ImageSkia();
    }

    app_list::SuggestionChipView* suggestion_chip_view =
        new app_list::SuggestionChipView(params, /*listener=*/this);

    // Given a suggestion chip view, we need to be able to look up the id of
    // the underlying suggestion. This is used for handling press events.
    suggestion_chip_view->set_id(id);

    // Given an id, we also want to be able to look up the corresponding
    // suggestion chip view. This is used for handling icon download events.
    suggestion_chip_views_[id] = suggestion_chip_view;

    contents_view_->AddChildView(suggestion_chip_view);
  }
  UpdateContentsBounds();
  SetVisible(contents_view_->has_children());
}

void SuggestionContainerView::OnSuggestionsCleared() {
  // Abort any download requests in progress.
  download_request_weak_factory_.InvalidateWeakPtrs();

  SetVisible(false);

  // When modifying the view hierarchy, make sure we keep our view cache synced.
  contents_view_->RemoveAllChildViews(/*delete_children=*/true);
  suggestion_chip_views_.clear();

  UpdateContentsBounds();
}

void SuggestionContainerView::OnSuggestionChipIconDownloaded(
    int id,
    const gfx::ImageSkia& icon) {
  if (!icon.isNull())
    suggestion_chip_views_[id]->SetIcon(icon);
}

void SuggestionContainerView::OnSuggestionChipPressed(
    app_list::SuggestionChipView* suggestion_chip_view) {
  assistant_controller_->OnSuggestionChipPressed(suggestion_chip_view->id());
}

void SuggestionContainerView::UpdateContentsBounds() {
  contents_view_->SetBounds(0, 0, contents_view_->GetPreferredSize().width(),
                            kPreferredHeightDip);
}

}  // namespace ash
