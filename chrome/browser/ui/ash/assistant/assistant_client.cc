// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/assistant/assistant_client.h"

#include <utility>

#include "ash/public/interfaces/voice_interaction_controller.mojom.h"
#include "chrome/browser/chromeos/arc/voice_interaction/voice_interaction_controller_client.h"
#include "chrome/browser/ui/ash/assistant/assistant_card_renderer.h"
#include "chrome/browser/ui/ash/assistant/assistant_image_downloader.h"
#include "chromeos/services/assistant/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {
// Owned by ChromeBrowserMainChromeOS:
AssistantClient* g_instance = nullptr;
}  // namespace

// static
AssistantClient* AssistantClient::Get() {
  DCHECK(g_instance);
  return g_instance;
}

AssistantClient::AssistantClient()
    : client_binding_(this),
      audio_input_binding_(&audio_input_),
      context_binding_(&context_) {
  DCHECK_EQ(nullptr, g_instance);
  g_instance = this;
}

AssistantClient::~AssistantClient() {
  DCHECK(g_instance);
  g_instance = nullptr;
  context_binding_.Close();
}

void AssistantClient::MaybeInit(service_manager::Connector* connector) {
  if (initialized_)
    return;

  initialized_ = true;
  connector->BindInterface(chromeos::assistant::mojom::kServiceName,
                           &assistant_connection_);
  chromeos::assistant::mojom::AudioInputPtr audio_input_ptr;
  audio_input_binding_.Bind(mojo::MakeRequest(&audio_input_ptr));

  chromeos::assistant::mojom::ClientPtr client_ptr;
  client_binding_.Bind(mojo::MakeRequest(&client_ptr));

  chromeos::assistant::mojom::ContextPtr context_ptr;
  context_binding_.Bind(mojo::MakeRequest(&context_ptr));

  assistant_connection_->Init(std::move(client_ptr), std::move(context_ptr),
                              std::move(audio_input_ptr));

  assistant_card_renderer_ = std::make_unique<AssistantCardRenderer>(connector);
  assistant_image_downloader_ =
      std::make_unique<AssistantImageDownloader>(connector);
}

void AssistantClient::OnAssistantStatusChanged(bool running) {
  arc::VoiceInteractionControllerClient::Get()->NotifyStatusChanged(
      running ? ash::mojom::VoiceInteractionState::RUNNING
              : ash::mojom::VoiceInteractionState::STOPPED);
}
