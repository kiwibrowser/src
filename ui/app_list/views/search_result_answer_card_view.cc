// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/search_result_answer_card_view.h"

#include <memory>
#include <utility>
#include <vector>

#include "ash/public/cpp/app_list/answer_card_contents_registry.h"
#include "ash/public/cpp/app_list/app_list_constants.h"
#include "base/bind.h"
#include "base/feature_list.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "ui/accessibility/ax_node.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/app_list/app_list_metrics.h"
#include "ui/app_list/app_list_view_delegate.h"
#include "ui/app_list/views/search_result_base_view.h"
#include "ui/base/ui_base_features.h"
#include "ui/gfx/canvas.h"
#include "ui/views/background.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/mus/remote_view/remote_view_host.h"

namespace app_list {

namespace {

// Helper to get/create answer card view by token.
views::View* GetViewByToken(
    const base::Optional<base::UnguessableToken>& token) {
  // Bail for invalid token.
  if (!token.has_value() || token->is_empty())
    return nullptr;

  // Use AnswerCardContentsRegistry for an in-process token-to-view map. See
  // answer_card_contents_registry.h. Null check because it could be missing in
  // Mash and for tests.
  if (AnswerCardContentsRegistry::Get())
    return AnswerCardContentsRegistry::Get()->GetView(token.value());

  // Use RemoteViewHost to embed the answer card contents provided in the
  // browser process in Mash.
  if (base::FeatureList::IsEnabled(features::kMash)) {
    views::RemoteViewHost* view = new views::RemoteViewHost();
    view->EmbedUsingToken(token.value(),
                          ui::mojom::kEmbedFlagEmbedderInterceptsEvents |
                              ui::mojom::kEmbedFlagEmbedderControlsVisibility,
                          base::DoNothing());
    return view;
  }

  return nullptr;
}

}  // namespace

// Container of the search answer view.
class SearchResultAnswerCardView::SearchAnswerContainerView
    : public SearchResultBaseView {
 public:
  explicit SearchAnswerContainerView(AppListViewDelegate* view_delegate)
      : view_delegate_(view_delegate) {
    SetFocusBehavior(FocusBehavior::ALWAYS);
    // Center the card horizontally in the container. Padding is set on the
    // server.
    auto answer_container_layout =
        std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal);
    answer_container_layout->set_main_axis_alignment(
        views::BoxLayout::MAIN_AXIS_ALIGNMENT_START);
    SetLayoutManager(std::move(answer_container_layout));
  }

  ~SearchAnswerContainerView() override {
    if (search_result_)
      search_result_->RemoveObserver(this);
  }

  bool SetSearchResult(SearchResult* search_result) {
    const base::Optional<base::UnguessableToken> old_token =
        search_result_ ? search_result_->answer_card_contents_token()
                       : base::nullopt;
    const base::Optional<base::UnguessableToken> new_token =
        search_result ? search_result->answer_card_contents_token()
                      : base::nullopt;

    views::View* result_view = child_count() ? child_at(0) : nullptr;
    if (old_token != new_token) {
      RemoveAllChildViews(true /* delete_children */);

      result_view = GetViewByToken(new_token);
      if (result_view)
        AddChildView(result_view);
    }

    base::string16 old_title;
    base::string16 new_title;
    if (search_result_) {
      search_result_->RemoveObserver(this);
      old_title = search_result_->title();
    }
    search_result_ = search_result;
    if (search_result_) {
      search_result_->AddObserver(this);
      if (result_view)
        result_view->SetPreferredSize(search_result_->answer_card_size());

      new_title = search_result_->title();
      SetAccessibleName(new_title);
    }

    return old_title != new_title;
  }

  // views::Button overrides:
  const char* GetClassName() const override {
    return "SearchAnswerContainerView";
  }

  void OnBlur() override { SetBackgroundHighlighted(false); }

  void OnFocus() override {
    ScrollRectToVisible(GetLocalBounds());
    NotifyAccessibilityEvent(ax::mojom::Event::kSelection, true);
    SetBackgroundHighlighted(true);
  }

  bool OnKeyPressed(const ui::KeyEvent& event) override {
    if (event.key_code() == ui::VKEY_SPACE) {
      // Shouldn't eat Space; we want Space to go to the search box.
      return false;
    }

    return Button::OnKeyPressed(event);
  }

  void GetAccessibleNodeData(ui::AXNodeData* node_data) override {
    // Default button role is atomic for ChromeVox, so assign a generic
    // container role to allow accessibility focus to get into this view.
    node_data->role = ax::mojom::Role::kGenericContainer;
    node_data->SetName(accessible_name());
  }

  void PaintButtonContents(gfx::Canvas* canvas) override {
    if (background_highlighted())
      canvas->FillRect(GetContentsBounds(), kAnswerCardSelectedColor);
  }

  // views::ButtonListener overrides:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override {
    DCHECK(sender == this);
    if (search_result_) {
      RecordSearchResultOpenSource(search_result_, view_delegate_->GetModel(),
                                   view_delegate_->GetSearchModel());
      view_delegate_->OpenSearchResult(search_result_->id(), event.flags());
    }
  }

  // SearchResultObserver overrides:
  void OnResultDestroying() override {
    RemoveAllChildViews(true /* delete_children */);
    search_result_ = nullptr;
  }

 private:
  AppListViewDelegate* const view_delegate_;  // Not owned.
  SearchResult* search_result_ = nullptr;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(SearchAnswerContainerView);
};

SearchResultAnswerCardView::SearchResultAnswerCardView(
    AppListViewDelegate* view_delegate)
    : search_answer_container_view_(
          new SearchAnswerContainerView(view_delegate)) {
  AddChildView(search_answer_container_view_);
  SetLayoutManager(std::make_unique<views::FillLayout>());
}

SearchResultAnswerCardView::~SearchResultAnswerCardView() {}

const char* SearchResultAnswerCardView::GetClassName() const {
  return "SearchResultAnswerCardView";
}

void SearchResultAnswerCardView::OnContainerSelected(
    bool from_bottom,
    bool directional_movement) {
  if (num_results() == 0)
    return;

  SetSelectedIndex(0);
}

int SearchResultAnswerCardView::GetYSize() {
  return num_results();
}

int SearchResultAnswerCardView::DoUpdate() {
  std::vector<SearchResult*> display_results =
      SearchModel::FilterSearchResultsByDisplayType(
          results(), ash::SearchResultDisplayType::kCard, 1);

  const bool have_result = !display_results.empty();

  const bool title_changed = search_answer_container_view_->SetSearchResult(
      have_result ? display_results[0] : nullptr);
  parent()->SetVisible(have_result);

  set_container_score(have_result ? display_results.front()->display_score()
                                  : 0);
  if (title_changed && search_answer_container_view_->HasFocus()) {
    search_answer_container_view_->NotifyAccessibilityEvent(
        ax::mojom::Event::kSelection, true);
  }
  return have_result ? 1 : 0;
}

void SearchResultAnswerCardView::UpdateSelectedIndex(int old_selected,
                                                     int new_selected) {
  // TODO(weidongg): This implementation is deprecated and should be removed as
  // part of removing "pseudo-focus" logic work (https://crbug.com/766807).
}

bool SearchResultAnswerCardView::OnKeyPressed(const ui::KeyEvent& event) {
  if (selected_index() == 0 &&
      search_answer_container_view_->OnKeyPressed(event)) {
    return true;
  }

  return SearchResultContainerView::OnKeyPressed(event);
}

views::View* SearchResultAnswerCardView::GetSelectedView() {
  // TODO(weidongg): This implementation is deprecated and should be removed as
  // part of removing "pseudo-focus" logic work (https://crbug.com/766807).
  return nullptr;
}

SearchResultBaseView* SearchResultAnswerCardView::GetFirstResultView() {
  return num_results() <= 0 ? nullptr : search_answer_container_view_;
}

views::View* SearchResultAnswerCardView::GetSearchAnswerContainerViewForTest()
    const {
  return search_answer_container_view_;
}

}  // namespace app_list
