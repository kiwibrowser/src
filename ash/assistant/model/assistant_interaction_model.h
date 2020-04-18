// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ASSISTANT_MODEL_ASSISTANT_INTERACTION_MODEL_H_
#define ASH_ASSISTANT_MODEL_ASSISTANT_INTERACTION_MODEL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/observer_list.h"
#include "chromeos/services/assistant/public/mojom/assistant.mojom.h"

namespace ash {

class AssistantInteractionModelObserver;
class AssistantQuery;
class AssistantUiElement;

// Enumeration of interaction input modalities.
enum class InputModality {
  kKeyboard,
  kStylus,
  kVoice,
};

// TODO(dmblack): This is an oversimplification. We will eventually want to
// distinctly represent listening/thinking/etc. states explicitly so they can
// be adequately represented in the UI.
// Enumeration of interaction states.
enum class InteractionState {
  kActive,
  kInactive,
};

// Enumeration of interaction mic states.
enum class MicState {
  kClosed,
  kOpen,
};

// Models the Assistant interaction. This includes query state, state of speech
// recognition, as well as renderable AssistantUiElements and suggestions.
class AssistantInteractionModel {
 public:
  using AssistantSuggestion = chromeos::assistant::mojom::AssistantSuggestion;
  using AssistantSuggestionPtr =
      chromeos::assistant::mojom::AssistantSuggestionPtr;

  AssistantInteractionModel();
  ~AssistantInteractionModel();

  // Adds/removes the specified interaction model |observer|.
  void AddObserver(AssistantInteractionModelObserver* observer);
  void RemoveObserver(AssistantInteractionModelObserver* observer);

  // Resets the interaction to its initial state.
  void ClearInteraction();

  // Sets the interaction state.
  void SetInteractionState(InteractionState interaction_state);

  // Returns the interaction state.
  InteractionState interaction_state() const { return interaction_state_; }

  // Updates the input modality for the interaction.
  void SetInputModality(InputModality input_modality);

  // Returns the input modality for the interaction.
  InputModality input_modality() const { return input_modality_; }

  // Updates the mic state for the interaction.
  void SetMicState(MicState mic_state);

  // Returns the mic state for the interaction.
  MicState mic_state() const { return mic_state_; }

  // Adds the specified |ui_element| that should be rendered for the
  // interaction.
  void AddUiElement(std::unique_ptr<AssistantUiElement> ui_element);

  // Clears all UI elements for the interaction.
  void ClearUiElements();

  // Updates the query for the interaction.
  void SetQuery(std::unique_ptr<AssistantQuery> query);

  // Returns the query for the interaction.
  const AssistantQuery& query() const { return *query_; }

  // Clears the query for the interaction.
  void ClearQuery();

  // Adds the specified |suggestions| that should be rendered for the
  // interaction.
  void AddSuggestions(std::vector<AssistantSuggestionPtr> suggestions);

  // Returns the suggestion uniquely identified by the specified |id|, or
  // |nullptr| if no matching suggestion is found.
  const AssistantSuggestion* GetSuggestionById(int id) const;

  // Clears all suggestions for the interaction.
  void ClearSuggestions();

 private:
  void NotifyInteractionStateChanged();
  void NotifyInputModalityChanged();
  void NotifyMicStateChanged();
  void NotifyUiElementAdded(const AssistantUiElement* ui_element);
  void NotifyUiElementsCleared();
  void NotifyQueryChanged();
  void NotifyQueryCleared();
  void NotifySuggestionsAdded(
      const std::map<int, AssistantSuggestion*>& suggestions);
  void NotifySuggestionsCleared();

  InteractionState interaction_state_ = InteractionState::kInactive;
  InputModality input_modality_ = InputModality::kVoice;
  MicState mic_state_ = MicState::kClosed;
  std::unique_ptr<AssistantQuery> query_;
  std::vector<AssistantSuggestionPtr> suggestions_;
  std::vector<std::unique_ptr<AssistantUiElement>> ui_element_list_;

  base::ObserverList<AssistantInteractionModelObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(AssistantInteractionModel);
};

}  // namespace ash

#endif  // ASH_ASSISTANT_MODEL_ASSISTANT_INTERACTION_MODEL_H_
