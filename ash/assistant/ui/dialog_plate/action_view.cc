// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/assistant/ui/dialog_plate/action_view.h"

#include <memory>

#include "ash/assistant/assistant_controller.h"
#include "ash/assistant/ui/logo_view/base_logo_view.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/layout/fill_layout.h"

namespace ash {

namespace {

// Appearance.
constexpr int kPreferredSizeDip = 20;

}  // namespace

ActionView::ActionView(AssistantController* assistant_controller,
                       ActionViewListener* listener)
    : assistant_controller_(assistant_controller),
      listener_(listener),
      keyboard_action_view_(new views::ImageView()),
      voice_action_view_(BaseLogoView::Create()) {
  InitLayout();
  UpdateState(/*animate=*/false);

  // The Assistant controller indirectly owns the view hierarchy to which
  // ActionView belongs so is guaranteed to outlive it.
  assistant_controller_->AddInteractionModelObserver(this);
}

ActionView::~ActionView() {
  assistant_controller_->RemoveInteractionModelObserver(this);
}

gfx::Size ActionView::CalculatePreferredSize() const {
  return gfx::Size(kPreferredSizeDip, kPreferredSizeDip);
}

void ActionView::InitLayout() {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  gfx::Size size = gfx::Size(kPreferredSizeDip, kPreferredSizeDip);

  // Keyboard action.
  keyboard_action_view_->SetImage(
      gfx::CreateVectorIcon(kSendIcon, kPreferredSizeDip, gfx::kGoogleBlue500));
  keyboard_action_view_->SetImageSize(size);
  keyboard_action_view_->SetPreferredSize(size);
  AddChildView(keyboard_action_view_);

  // Voice action.
  voice_action_view_->SetPreferredSize(size);
  AddChildView(voice_action_view_);
}

void ActionView::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() != ui::ET_GESTURE_TAP)
    return;

  event->SetHandled();

  if (listener_)
    listener_->OnActionPressed();
}

bool ActionView::OnMousePressed(const ui::MouseEvent& event) {
  if (listener_)
    listener_->OnActionPressed();
  return true;
}

void ActionView::OnInputModalityChanged(InputModality input_modality) {
  UpdateState(/*animate=*/false);
}

void ActionView::OnMicStateChanged(MicState mic_state) {
  UpdateState(/*animate=*/true);
}

void ActionView::UpdateState(bool animate) {
  const AssistantInteractionModel* interaction_model =
      assistant_controller_->interaction_model();

  InputModality input_modality = interaction_model->input_modality();

  // We don't need to handle stylus input modality.
  if (input_modality == InputModality::kStylus)
    return;

  if (input_modality == InputModality::kKeyboard) {
    voice_action_view_->SetVisible(false);
    keyboard_action_view_->SetVisible(true);
    return;
  }

  BaseLogoView::State mic_state;
  switch (interaction_model->mic_state()) {
    case MicState::kClosed:
      // Do not animate when the first time showing the LogoView in kMic state.
      // Only animate when changing from kListening or kUserSpeaks states.
      mic_state = BaseLogoView::State::kMicFab;
      break;
    case MicState::kOpen:
      mic_state = BaseLogoView::State::kListening;
      break;
  }
  voice_action_view_->SetState(mic_state, animate);

  keyboard_action_view_->SetVisible(false);
  voice_action_view_->SetVisible(true);
}

}  // namespace ash
