// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ASSISTANT_ASSISTANT_CONTROLLER_H_
#define ASH_ASSISTANT_ASSISTANT_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "ash/assistant/model/assistant_interaction_model.h"
#include "ash/assistant/model/assistant_interaction_model_observer.h"
#include "ash/highlighter/highlighter_controller.h"
#include "ash/public/interfaces/assistant_card_renderer.mojom.h"
#include "ash/public/interfaces/assistant_controller.mojom.h"
#include "ash/public/interfaces/assistant_image_downloader.mojom.h"
#include "base/macros.h"
#include "chromeos/services/assistant/public/mojom/assistant.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "ui/gfx/geometry/rect.h"

namespace base {
class UnguessableToken;
}  // namespace base

namespace ash {

class AssistantBubble;
class AssistantInteractionModelObserver;

class AssistantController
    : public mojom::AssistantController,
      public chromeos::assistant::mojom::AssistantEventSubscriber,
      public AssistantInteractionModelObserver,
      public HighlighterController::Observer {
 public:
  using AssistantSuggestion = chromeos::assistant::mojom::AssistantSuggestion;
  using AssistantSuggestionPtr =
      chromeos::assistant::mojom::AssistantSuggestionPtr;
  using AssistantInteractionResolution =
      chromeos::assistant::mojom::AssistantInteractionResolution;

  AssistantController();
  ~AssistantController() override;

  void BindRequest(mojom::AssistantControllerRequest request);

  // Returns a reference to the underlying interaction model.
  const AssistantInteractionModel* interaction_model() const {
    return &assistant_interaction_model_;
  }

  // Registers the specified |observer| with the interaction model observer
  // pool.
  void AddInteractionModelObserver(AssistantInteractionModelObserver* observer);

  // Unregisters the specified |observer| from the interaction model observer
  // pool.
  void RemoveInteractionModelObserver(
      AssistantInteractionModelObserver* observer);

  // Renders a card, uniquely identified by |id_token|, according to the
  // specified |params|. When the card is ready for embedding, the supplied
  // |callback| is run with a token for embedding.
  void RenderCard(const base::UnguessableToken& id_token,
                  mojom::AssistantCardParamsPtr params,
                  mojom::AssistantCardRenderer::RenderCallback callback);

  // Releases resources for the card uniquely identified by |id_token|.
  void ReleaseCard(const base::UnguessableToken& id_token);

  // Releases resources for any card uniquely identified in |id_token_list|.
  void ReleaseCards(const std::vector<base::UnguessableToken>& id_tokens);

  // Downloads the image found at the specified |url|. On completion, the
  // supplied |callback| will be run with the downloaded image. If the download
  // attempt is unsuccessful, a NULL image is returned.
  void DownloadImage(
      const GURL& url,
      mojom::AssistantImageDownloader::DownloadCallback callback);

  // Invoke to modify the Assistant interaction state.
  void StartInteraction();
  void StopInteraction();
  void ToggleInteraction();

  // Invoked on dialog plate action pressed event.
  void OnDialogPlateActionPressed(const std::string& text);

  // Invoked on dialog plate contents changed event.
  void OnDialogPlateContentsChanged(const std::string& text);

  // Invoked on dialog plate contents committed event.
  void OnDialogPlateContentsCommitted(const std::string& text);

  // Invoked on suggestion chip pressed event.
  void OnSuggestionChipPressed(int id);

  // AssistantInteractionModelObserver:
  void OnInputModalityChanged(InputModality input_modality) override;
  void OnInteractionStateChanged(InteractionState interaction_state) override;

  // HighlighterController::Observer:
  void OnHighlighterEnabledChanged(HighlighterEnabledState state) override;

  // chromeos::assistant::mojom::AssistantEventSubscriber:
  void OnInteractionStarted() override;
  void OnInteractionFinished(
      AssistantInteractionResolution resolution) override;
  void OnHtmlResponse(const std::string& response) override;
  void OnSuggestionsResponse(
      std::vector<AssistantSuggestionPtr> response) override;
  void OnTextResponse(const std::string& response) override;
  void OnOpenUrlResponse(const GURL& url) override;
  void OnSpeechRecognitionStarted() override;
  void OnSpeechRecognitionIntermediateResult(
      const std::string& high_confidence_text,
      const std::string& low_confidence_text) override;
  void OnSpeechRecognitionEndOfUtterance() override;
  void OnSpeechRecognitionFinalResult(const std::string& final_result) override;
  void OnSpeechLevelUpdated(float speech_level) override;

  // mojom::AssistantController:
  void SetAssistant(
      chromeos::assistant::mojom::AssistantPtr assistant) override;
  void SetAssistantCardRenderer(
      mojom::AssistantCardRendererPtr assistant_card_renderer) override;
  void SetAssistantImageDownloader(
      mojom::AssistantImageDownloaderPtr assistant_image_downloader) override;
  void RequestScreenshot(const gfx::Rect& rect,
                         RequestScreenshotCallback callback) override;
  void OnCardPressed(const GURL& url) override;

 private:
  mojo::BindingSet<mojom::AssistantController> assistant_controller_bindings_;
  mojo::Binding<chromeos::assistant::mojom::AssistantEventSubscriber>
      assistant_event_subscriber_binding_;
  AssistantInteractionModel assistant_interaction_model_;

  chromeos::assistant::mojom::AssistantPtr assistant_;
  mojom::AssistantCardRendererPtr assistant_card_renderer_;
  mojom::AssistantImageDownloaderPtr assistant_image_downloader_;

  std::unique_ptr<AssistantBubble> assistant_bubble_;

  DISALLOW_COPY_AND_ASSIGN(AssistantController);
};

}  // namespace ash

#endif  // ASH_ASSISTANT_ASSISTANT_CONTROLLER_H_
