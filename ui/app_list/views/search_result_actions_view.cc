// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/search_result_actions_view.h"

#include <stddef.h>

#include <algorithm>

#include "ui/app_list/views/search_result_actions_view_delegate.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/box_layout.h"

namespace app_list {

SearchResultActionsView::SearchResultActionsView(
    SearchResultActionsViewDelegate* delegate)
    : delegate_(delegate),
      selected_action_(-1) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kHorizontal, gfx::Insets(10, 0), 0));
}

SearchResultActionsView::~SearchResultActionsView() {}

void SearchResultActionsView::SetActions(const SearchResult::Actions& actions) {
  RemoveAllChildViews(true);

  for (size_t i = 0; i < actions.size(); ++i) {
    const SearchResult::Action& action = actions.at(i);
    if (action.label_text.empty())
      CreateImageButton(action);
    else
      CreateBlueButton(action);
  }

  PreferredSizeChanged();
  SetSelectedAction(-1);
}

void SearchResultActionsView::SetSelectedAction(int action_index) {
  // Clamp |action_index| in [-1, child_count()].
  action_index = std::min(child_count(), std::max(-1, action_index));

  if (selected_action_ == action_index)
    return;

  selected_action_ = action_index;
  SchedulePaint();

  if (IsValidActionIndex(selected_action_)) {
    child_at(selected_action_)
        ->NotifyAccessibilityEvent(ax::mojom::Event::kHover, true);
  }
}

bool SearchResultActionsView::IsValidActionIndex(int action_index) const {
  return action_index >= 0 && action_index < child_count();
}

void SearchResultActionsView::CreateImageButton(
    const SearchResult::Action& action) {
  views::ImageButton* button = new views::ImageButton(this);
  button->SetBorder(views::CreateEmptyBorder(0, 9, 0, 9));
  button->SetAccessibleName(action.tooltip_text);
  button->SetImageAlignment(views::ImageButton::ALIGN_CENTER,
                            views::ImageButton::ALIGN_MIDDLE);
  button->SetImage(views::Button::STATE_NORMAL, &action.base_image);
  button->SetImage(views::Button::STATE_HOVERED, &action.hover_image);
  button->SetImage(views::Button::STATE_PRESSED, &action.pressed_image);
  button->SetTooltipText(action.tooltip_text);
  AddChildView(button);
}

void SearchResultActionsView::CreateBlueButton(
    const SearchResult::Action& action) {
  views::MdTextButton* button =
      views::MdTextButton::Create(this, action.label_text);
  button->SetProminent(true);
  button->SetTooltipText(action.tooltip_text);
  button->SetFocusBehavior(FocusBehavior::NEVER);
  AddChildView(button);
}

void SearchResultActionsView::OnPaint(gfx::Canvas* canvas) {
  if (!IsValidActionIndex(selected_action_))
    return;

  const gfx::Rect active_action_bounds(child_at(selected_action_)->bounds());
  const SkColor kActiveActionBackground = SkColorSetRGB(0xA0, 0xA0, 0xA0);
  canvas->FillRect(active_action_bounds, kActiveActionBackground);
}

void SearchResultActionsView::ButtonPressed(views::Button* sender,
                                            const ui::Event& event) {
  if (!delegate_)
    return;

  const int index = GetIndexOf(sender);
  DCHECK_NE(-1, index);
  delegate_->OnSearchResultActionActivated(index, event.flags());
}

}  // namespace app_list
