// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ASSISTANT_UI_DIALOG_PLATE_ACTION_VIEW_H_
#define ASH_ASSISTANT_UI_DIALOG_PLATE_ACTION_VIEW_H_

#include "ash/assistant/model/assistant_interaction_model_observer.h"
#include "base/macros.h"
#include "ui/views/view.h"

namespace views {
class ImageView;
}  // namespace views

namespace ash {

class ActionView;
class AssistantController;
class BaseLogoView;

// Listener which receives notification of action view events.
class ActionViewListener {
 public:
  // Invoked when the action is pressed.
  virtual void OnActionPressed() = 0;

 protected:
  virtual ~ActionViewListener() = default;
};

// A stateful view belonging to DialogPlate which indicates current user input
// modality and delivers notification of press events.
class ActionView : public views::View,
                   public AssistantInteractionModelObserver {
 public:
  ActionView(AssistantController* assistant_controller,
             ActionViewListener* listener);
  ~ActionView() override;

  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;

  // AssistantInteractionModelObserver:
  void OnInputModalityChanged(InputModality input_modality) override;
  void OnMicStateChanged(MicState mic_state) override;

 private:
  void InitLayout();

  // If |animate| is false, there is no exit animation of current state and
  // enter animation of the next state of the LogoView.
  void UpdateState(bool animate);

  AssistantController* const assistant_controller_;  // Owned by Shell.
  ActionViewListener* listener_;

  views::ImageView* keyboard_action_view_;  // Owned by view hierarchy.
  BaseLogoView* voice_action_view_;         // Owned by view hierarchy.

  DISALLOW_COPY_AND_ASSIGN(ActionView);
};

}  // namespace ash

#endif  // ASH_ASSISTANT_UI_DIALOG_PLATE_ACTION_VIEW_H_
