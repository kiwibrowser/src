// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ASSISTANT_UI_SUGGESTION_CONTAINER_VIEW_H_
#define ASH_ASSISTANT_UI_SUGGESTION_CONTAINER_VIEW_H_

#include <map>

#include "ash/assistant/model/assistant_interaction_model_observer.h"
#include "base/macros.h"
#include "ui/app_list/views/suggestion_chip_view.h"
#include "ui/views/controls/scroll_view.h"

namespace ash {

class AssistantController;

class SuggestionContainerView : public views::ScrollView,
                                public AssistantInteractionModelObserver,
                                public app_list::SuggestionChipListener {
 public:
  using AssistantSuggestion = chromeos::assistant::mojom::AssistantSuggestion;

  explicit SuggestionContainerView(AssistantController* assistant_controller);
  ~SuggestionContainerView() override;

  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  int GetHeightForWidth(int width) const override;

  // AssistantInteractionModelObserver:
  void OnSuggestionsAdded(
      const std::map<int, AssistantSuggestion*>& suggestions) override;
  void OnSuggestionsCleared() override;

  // app_list::SuggestionChipListener:
  void OnSuggestionChipPressed(
      app_list::SuggestionChipView* suggestion_chip_view) override;

 private:
  void InitLayout();
  void UpdateContentsBounds();

  // Invoked on suggestion chip icon downloaded event.
  void OnSuggestionChipIconDownloaded(int id, const gfx::ImageSkia& icon);

  AssistantController* const assistant_controller_;  // Owned by Shell.
  views::View* contents_view_;                       // Owned by view hierarchy.

  // Cache of suggestion chip views owned by the view hierarchy. The key for the
  // map is the unique identifier by which the Assistant interaction model
  // identifies the view's underlying suggestion.
  std::map<int, app_list::SuggestionChipView*> suggestion_chip_views_;

  // Weak pointer factory used for image downloading requests.
  base::WeakPtrFactory<SuggestionContainerView> download_request_weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SuggestionContainerView);
};

}  // namespace ash

#endif  // ASH_ASSISTANT_UI_SUGGESTION_CONTAINER_VIEW_H_
