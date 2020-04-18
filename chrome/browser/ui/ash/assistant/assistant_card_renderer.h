// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_ASSISTANT_ASSISTANT_CARD_RENDERER_H_
#define CHROME_BROWSER_UI_ASH_ASSISTANT_ASSISTANT_CARD_RENDERER_H_

#include <unordered_map>
#include <vector>

#include "ash/public/interfaces/assistant_card_renderer.mojom.h"
#include "base/macros.h"
#include "base/unguessable_token.h"
#include "mojo/public/cpp/bindings/binding.h"

class AccountId;
class GURL;

namespace service_manager {
class Connector;
}  // namespace service_manager

namespace {
class AssistantCard;
}  // namespace

// AssistantCardRenderer is the class responsible for rendering Assistant cards
// and owning their associated resources in chrome/browser for views to embed.
// In order to ensure resources live only as long as is necessary, any call to
// Render should be paired with a corresponding call to Release when the card
// is no longer needed. As such, the caller of Render must provide an identifier
// by which to uniquely identify a card.
class AssistantCardRenderer : public ash::mojom::AssistantCardRenderer {
 public:
  explicit AssistantCardRenderer(service_manager::Connector* connector);
  ~AssistantCardRenderer() override;

  // ash::mojom::AssistantCardRenderer:
  void Render(
      const AccountId& account_id,
      const base::UnguessableToken& id_token,
      ash::mojom::AssistantCardParamsPtr params,
      ash::mojom::AssistantCardRenderer::RenderCallback callback) override;
  void Release(const base::UnguessableToken& id_token) override;
  void ReleaseAll(
      const std::vector<base::UnguessableToken>& id_tokens) override;

  // Invoked on card pressed event to handle navigating to |url|.
  void OnCardPressed(const GURL& url);

 private:
  service_manager::Connector* const connector_;
  mojo::Binding<ash::mojom::AssistantCardRenderer> binding_;

  std::unordered_map<base::UnguessableToken,
                     std::unique_ptr<AssistantCard>,
                     base::UnguessableTokenHash>
      assistant_cards_;

  DISALLOW_COPY_AND_ASSIGN(AssistantCardRenderer);
};

#endif  // CHROME_BROWSER_UI_ASH_ASSISTANT_ASSISTANT_CARD_RENDERER_H_
