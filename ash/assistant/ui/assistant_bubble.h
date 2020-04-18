// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ASSISTANT_UI_ASSISTANT_BUBBLE_H_
#define ASH_ASSISTANT_UI_ASSISTANT_BUBBLE_H_

#include "ash/assistant/model/assistant_interaction_model_observer.h"
#include "base/macros.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class Widget;
}  // namespace views

namespace ash {

class AssistantController;

namespace {
class AssistantContainerView;
}  // namespace

class AssistantBubble : public views::WidgetObserver,
                        public AssistantInteractionModelObserver {
 public:
  explicit AssistantBubble(AssistantController* assistant_controller);
  ~AssistantBubble() override;

  // views::WidgetObserver:
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  void OnWidgetDestroying(views::Widget* widget) override;

  // AssistantInteractionModelObserver:
  void OnInteractionStateChanged(InteractionState interaction_state) override;

 private:
  void Show();
  void Dismiss();

  AssistantController* const assistant_controller_;  // Owned by Shell.

  // Owned by view hierarchy.
  AssistantContainerView* container_view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AssistantBubble);
};

}  // namespace ash

#endif  // ASH_ASSISTANT_UI_ASSISTANT_BUBBLE_H_
