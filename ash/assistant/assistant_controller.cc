// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/assistant/assistant_controller.h"

#include "ash/assistant/model/assistant_interaction_model_observer.h"
#include "ash/assistant/model/assistant_query.h"
#include "ash/assistant/model/assistant_ui_element.h"
#include "ash/assistant/ui/assistant_bubble.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/shell_delegate.h"
#include "ash/system/toast/toast_data.h"
#include "ash/system/toast/toast_manager.h"
#include "base/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/unguessable_token.h"
#include "ui/snapshot/snapshot.h"

namespace ash {

namespace {

// Toast -----------------------------------------------------------------------

constexpr int kToastDurationMs = 2500;
constexpr char kUnboundServiceToastId[] =
    "assistant_controller_unbound_service";

// TODO(b/77638210): Localize string.
constexpr char kSomethingWentWrong[] =
    "Something went wrong. Try again in a few seconds.";

void ShowToast(const std::string& id, const std::string& text) {
  ToastData toast(id, base::UTF8ToUTF16(text), kToastDurationMs, base::nullopt);
  Shell::Get()->toast_manager()->Show(toast);
}

}  // namespace

// AssistantController ---------------------------------------------------------

AssistantController::AssistantController()
    : assistant_event_subscriber_binding_(this),
      assistant_bubble_(std::make_unique<AssistantBubble>(this)) {
  AddInteractionModelObserver(this);
  Shell::Get()->highlighter_controller()->AddObserver(this);
}

AssistantController::~AssistantController() {
  Shell::Get()->highlighter_controller()->RemoveObserver(this);
  RemoveInteractionModelObserver(this);

  assistant_controller_bindings_.CloseAllBindings();
  assistant_event_subscriber_binding_.Close();
}

void AssistantController::BindRequest(
    mojom::AssistantControllerRequest request) {
  assistant_controller_bindings_.AddBinding(this, std::move(request));
}

void AssistantController::SetAssistant(
    chromeos::assistant::mojom::AssistantPtr assistant) {
  assistant_ = std::move(assistant);

  // Subscribe to Assistant events.
  chromeos::assistant::mojom::AssistantEventSubscriberPtr ptr;
  assistant_event_subscriber_binding_.Bind(mojo::MakeRequest(&ptr));
  assistant_->AddAssistantEventSubscriber(std::move(ptr));
}

void AssistantController::SetAssistantCardRenderer(
    mojom::AssistantCardRendererPtr assistant_card_renderer) {
  assistant_card_renderer_ = std::move(assistant_card_renderer);
}

void AssistantController::SetAssistantImageDownloader(
    mojom::AssistantImageDownloaderPtr assistant_image_downloader) {
  assistant_image_downloader_ = std::move(assistant_image_downloader);
}

void AssistantController::RequestScreenshot(
    const gfx::Rect& rect,
    RequestScreenshotCallback callback) {
  // TODO(muyuanli): handle multi-display when assistant's behavior is defined.
  auto* root_window = Shell::GetPrimaryRootWindow();
  gfx::Rect source_rect =
      rect.IsEmpty() ? gfx::Rect(root_window->bounds().size()) : rect;
  ui::GrabWindowSnapshotAsyncJPEG(
      root_window, source_rect,
      base::BindRepeating(
          [](RequestScreenshotCallback callback,
             scoped_refptr<base::RefCountedMemory> data) {
            std::move(callback).Run(std::vector<uint8_t>(
                data->front(), data->front() + data->size()));
          },
          base::Passed(&callback)));
}

void AssistantController::RenderCard(
    const base::UnguessableToken& id_token,
    mojom::AssistantCardParamsPtr params,
    mojom::AssistantCardRenderer::RenderCallback callback) {
  DCHECK(assistant_card_renderer_);

  const mojom::UserSession* user_session =
      Shell::Get()->session_controller()->GetUserSession(0);

  if (!user_session) {
    LOG(WARNING) << "Unable to retrieve active user session.";
    return;
  }

  AccountId account_id = user_session->user_info->account_id;
  assistant_card_renderer_->Render(account_id, id_token, std::move(params),
                                   std::move(callback));
}

void AssistantController::ReleaseCard(const base::UnguessableToken& id_token) {
  DCHECK(assistant_card_renderer_);
  assistant_card_renderer_->Release(id_token);
}

void AssistantController::ReleaseCards(
    const std::vector<base::UnguessableToken>& id_tokens) {
  DCHECK(assistant_card_renderer_);
  assistant_card_renderer_->ReleaseAll(id_tokens);
}

void AssistantController::DownloadImage(
    const GURL& url,
    mojom::AssistantImageDownloader::DownloadCallback callback) {
  DCHECK(assistant_image_downloader_);

  const mojom::UserSession* user_session =
      Shell::Get()->session_controller()->GetUserSession(0);

  if (!user_session) {
    LOG(WARNING) << "Unable to retrieve active user session.";
    return;
  }

  AccountId account_id = user_session->user_info->account_id;
  assistant_image_downloader_->Download(account_id, url, std::move(callback));
}

void AssistantController::AddInteractionModelObserver(
    AssistantInteractionModelObserver* observer) {
  assistant_interaction_model_.AddObserver(observer);
}

void AssistantController::RemoveInteractionModelObserver(
    AssistantInteractionModelObserver* observer) {
  assistant_interaction_model_.RemoveObserver(observer);
}

void AssistantController::StartInteraction() {
  if (!assistant_) {
    ShowToast(kUnboundServiceToastId, kSomethingWentWrong);
    return;
  }
  OnInteractionStarted();
}

void AssistantController::StopInteraction() {
  assistant_interaction_model_.SetInteractionState(InteractionState::kInactive);
}

void AssistantController::ToggleInteraction() {
  if (assistant_interaction_model_.interaction_state() ==
      InteractionState::kInactive) {
    StartInteraction();
  } else {
    StopInteraction();
  }
}

void AssistantController::OnInteractionStateChanged(
    InteractionState interaction_state) {
  if (interaction_state == InteractionState::kActive)
    return;

  // When the user-facing interaction is dismissed, we instruct the service to
  // terminate any listening, speaking, or query in flight.
  DCHECK(assistant_);
  assistant_->StopActiveInteraction();

  assistant_interaction_model_.ClearInteraction();
  assistant_interaction_model_.SetInputModality(InputModality::kVoice);
}

void AssistantController::OnHighlighterEnabledChanged(
    HighlighterEnabledState state) {
  assistant_interaction_model_.SetInputModality(InputModality::kStylus);
  if (state == HighlighterEnabledState::kEnabled) {
    assistant_interaction_model_.SetInteractionState(InteractionState::kActive);
  } else if (state == HighlighterEnabledState::kDisabledByUser) {
    assistant_interaction_model_.SetInteractionState(
        InteractionState::kInactive);
  }
}

void AssistantController::OnInputModalityChanged(InputModality input_modality) {
  if (input_modality == InputModality::kVoice)
    return;

  // When switching to a non-voice input modality we instruct the underlying
  // service to terminate any listening, speaking, or in flight voice query. We
  // do not do this when switching to voice input modality because initiation of
  // a voice interaction will automatically interrupt any pre-existing activity.
  // Stopping the active interaction here for voice input modality would
  // actually have the undesired effect of stopping the voice interaction.
  if (assistant_interaction_model_.query().type() ==
      AssistantQueryType::kVoice) {
    DCHECK(assistant_);
    assistant_->StopActiveInteraction();
  }
}

void AssistantController::OnInteractionStarted() {
  assistant_interaction_model_.SetInteractionState(InteractionState::kActive);
}

void AssistantController::OnInteractionFinished(
    AssistantInteractionResolution resolution) {
  // When a voice query is interrupted we do not receive any follow up speech
  // recognition events but the mic is closed.
  if (resolution == AssistantInteractionResolution::kInterruption) {
    assistant_interaction_model_.SetMicState(MicState::kClosed);
  }
}

void AssistantController::OnCardPressed(const GURL& url) {
  OnOpenUrlResponse(url);
}

void AssistantController::OnDialogPlateActionPressed(const std::string& text) {
  InputModality input_modality = assistant_interaction_model_.input_modality();

  // When using keyboard input modality, pressing the dialog plate action is
  // equivalent to a commit.
  if (input_modality == InputModality::kKeyboard) {
    OnDialogPlateContentsCommitted(text);
    return;
  }

  DCHECK(assistant_);

  // It should not be possible to press the dialog plate action when not using
  // keyboard or voice input modality.
  DCHECK(input_modality == InputModality::kVoice);

  // When using voice input modality, pressing the dialog plate action will
  // toggle the voice interaction state.
  switch (assistant_interaction_model_.mic_state()) {
    case MicState::kClosed:
      assistant_->StartVoiceInteraction();
      break;
    case MicState::kOpen:
      assistant_->StopActiveInteraction();
      break;
  }
}

void AssistantController::OnDialogPlateContentsChanged(
    const std::string& text) {
  if (text.empty()) {
    // Note: This does not open the mic. It only updates the input modality to
    // voice so that we will show the mic icon in the UI.
    assistant_interaction_model_.SetInputModality(InputModality::kVoice);
  } else {
    assistant_interaction_model_.SetInputModality(InputModality::kKeyboard);
    assistant_interaction_model_.SetMicState(MicState::kClosed);
  }
}

void AssistantController::OnDialogPlateContentsCommitted(
    const std::string& text) {
  // TODO(dmblack): Handle an empty text query more gracefully by showing a
  // helpful message to the user. Currently we just reset state and pretend as
  // if nothing happened.
  if (text.empty()) {
    assistant_interaction_model_.ClearInteraction();
    assistant_interaction_model_.SetInputModality(InputModality::kVoice);
    return;
  }

  assistant_interaction_model_.ClearInteraction();
  assistant_interaction_model_.SetQuery(
      std::make_unique<AssistantTextQuery>(text));

  // Note: This does not open the mic. It only updates the input modality to
  // voice so that we will show the mic icon in the UI.
  assistant_interaction_model_.SetInputModality(InputModality::kVoice);

  DCHECK(assistant_);
  assistant_->SendTextQuery(text);
}

void AssistantController::OnHtmlResponse(const std::string& response) {
  assistant_interaction_model_.AddUiElement(
      std::make_unique<AssistantCardElement>(response));
}

void AssistantController::OnSuggestionChipPressed(int id) {
  const AssistantSuggestion* suggestion =
      assistant_interaction_model_.GetSuggestionById(id);

  DCHECK(suggestion);

  // If the suggestion contains a non-empty action url, we will handle the
  // suggestion chip pressed event by launching the action url in the browser.
  if (!suggestion->action_url.is_empty()) {
    OnOpenUrlResponse(suggestion->action_url);
    return;
  }

  // Otherwise, we will submit a simple text query using the suggestion text.
  const std::string text = suggestion->text;

  assistant_interaction_model_.ClearInteraction();
  assistant_interaction_model_.SetQuery(
      std::make_unique<AssistantTextQuery>(text));

  DCHECK(assistant_);
  assistant_->SendTextQuery(text);
}

void AssistantController::OnSuggestionsResponse(
    std::vector<AssistantSuggestionPtr> response) {
  assistant_interaction_model_.AddSuggestions(std::move(response));
}

void AssistantController::OnTextResponse(const std::string& response) {
  assistant_interaction_model_.AddUiElement(
      std::make_unique<AssistantTextElement>(response));
}

void AssistantController::OnSpeechRecognitionStarted() {
  assistant_interaction_model_.ClearInteraction();
  assistant_interaction_model_.SetInputModality(InputModality::kVoice);
  assistant_interaction_model_.SetMicState(MicState::kOpen);
  assistant_interaction_model_.SetQuery(
      std::make_unique<AssistantVoiceQuery>());
}

void AssistantController::OnSpeechRecognitionIntermediateResult(
    const std::string& high_confidence_text,
    const std::string& low_confidence_text) {
  assistant_interaction_model_.SetQuery(std::make_unique<AssistantVoiceQuery>(
      high_confidence_text, low_confidence_text));
}

void AssistantController::OnSpeechRecognitionEndOfUtterance() {
  assistant_interaction_model_.SetMicState(MicState::kClosed);
}

void AssistantController::OnSpeechRecognitionFinalResult(
    const std::string& final_result) {
  assistant_interaction_model_.SetQuery(
      std::make_unique<AssistantVoiceQuery>(final_result));
}

void AssistantController::OnSpeechLevelUpdated(float speech_level) {
  // TODO(dmblack): Handle.
  NOTIMPLEMENTED();
}

void AssistantController::OnOpenUrlResponse(const GURL& url) {
  Shell::Get()->shell_delegate()->OpenUrlFromArc(url);
  StopInteraction();
}

}  // namespace ash
