// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_ASSISTANT_ASSISTANT_CLIENT_H_
#define CHROME_BROWSER_UI_ASH_ASSISTANT_ASSISTANT_CLIENT_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/ash/assistant/assistant_context.h"
#include "chrome/browser/ui/ash/assistant/platform_audio_input_host.h"
#include "chromeos/services/assistant/public/mojom/assistant.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace service_manager {
class Connector;
}  // namespace service_manager

class AssistantCardRenderer;
class AssistantImageDownloader;

// Class to handle all assistant in-browser-process functionalities.
class AssistantClient : chromeos::assistant::mojom::Client {
 public:
  static AssistantClient* Get();

  AssistantClient();
  ~AssistantClient() override;

  void MaybeInit(service_manager::Connector* connector);

  // assistant::mojom::Client overrides:
  void OnAssistantStatusChanged(bool running) override;

 private:
  mojo::Binding<chromeos::assistant::mojom::Client> client_binding_;

  chromeos::assistant::mojom::AssistantPlatformPtr assistant_connection_;
  mojo::Binding<chromeos::assistant::mojom::AudioInput> audio_input_binding_;

  mojo::Binding<chromeos::assistant::mojom::Context> context_binding_;

  PlatformAudioInputHost audio_input_;

  AssistantContext context_;

  std::unique_ptr<AssistantCardRenderer> assistant_card_renderer_;
  std::unique_ptr<AssistantImageDownloader> assistant_image_downloader_;

  bool initialized_ = false;

  DISALLOW_COPY_AND_ASSIGN(AssistantClient);
};

#endif  // CHROME_BROWSER_UI_ASH_ASSISTANT_ASSISTANT_CLIENT_H_
