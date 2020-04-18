// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/assistant/assistant_card_renderer.h"

#include <memory>

#include "ash/public/cpp/app_list/answer_card_contents_registry.h"
#include "ash/public/interfaces/assistant_controller.mojom.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "base/optional.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/renderer_preferences.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/views/controls/webview/web_contents_set_background_color.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/view.h"

namespace {

constexpr char kDataUriPrefix[] = "data:text/html,";

// AssistantCard ---------------------------------------------------------------

class AssistantCard : public content::WebContentsDelegate,
                      public content::WebContentsObserver {
 public:
  AssistantCard(AssistantCardRenderer* assistant_card_renderer,
                const AccountId& account_id,
                ash::mojom::AssistantCardParamsPtr params,
                ash::mojom::AssistantCardRenderer::RenderCallback callback)
      : assistant_card_renderer_(assistant_card_renderer) {
    Profile* profile =
        chromeos::ProfileHelper::Get()->GetProfileByAccountId(account_id);

    if (!profile) {
      LOG(WARNING) << "Unable to retrieve profile for account_id.";
      return;
    }

    InitWebContents(profile, std::move(params));
    HandleWebContents(profile, std::move(callback));
  }

  ~AssistantCard() override {
    web_contents_->SetDelegate(nullptr);

    // When cards are rendered in the same process as ash, we need to release
    // the associated view registered in the AnswerCardContentsRegistry's
    // token-to-view map.
    if (app_list::AnswerCardContentsRegistry::Get() && embed_token_.has_value())
      app_list::AnswerCardContentsRegistry::Get()->Unregister(
          embed_token_.value());
  }

  // content::WebContentsDelegate:
  void ResizeDueToAutoResize(content::WebContents* web_contents,
                             const gfx::Size& new_size) override {
    web_view_->SetPreferredSize(new_size);
  }

  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override {
    assistant_card_renderer_->OnCardPressed(params.url);
    return nullptr;
  }

 private:
  void InitWebContents(Profile* profile,
                       ash::mojom::AssistantCardParamsPtr params) {
    web_contents_ =
        content::WebContents::Create(content::WebContents::CreateParams(
            profile, content::SiteInstance::Create(profile)));

    // Intercept navigation attempts so we can redirect to the browser.
    content::RendererPreferences* renderer_prefs =
        web_contents_->GetMutableRendererPrefs();
    renderer_prefs->browser_handles_all_top_level_requests = true;
    web_contents_->GetRenderViewHost()->SyncRendererPrefs();

    // Use a transparent background.
    views::WebContentsSetBackgroundColor::CreateForWebContentsWithColor(
        web_contents_.get(), SK_ColorTRANSPARENT);

    Observe(web_contents_.get());
    web_contents_->SetDelegate(this);

    // Load the card's HTML data string into the web contents.
    content::NavigationController::LoadURLParams load_params(
        GURL(kDataUriPrefix + params->html));
    load_params.should_clear_history_list = true;
    load_params.transition_type = ui::PAGE_TRANSITION_AUTO_TOPLEVEL;
    web_contents_->GetController().LoadURLWithParams(load_params);

    // Enable auto-resizing, respecting the specified size parameters.
    web_contents_->GetRenderWidgetHostView()->EnableAutoResize(
        gfx::Size(params->min_width_dip, 0),
        gfx::Size(params->max_width_dip, INT_MAX));
  }

  void HandleWebContents(
      Profile* profile,
      ash::mojom::AssistantCardRenderer::RenderCallback callback) {
    // When rendering cards in the same process as ash, we register the view for
    // the card with the AnswerCardContentsRegistry's token-to-view map. The
    // token returned from the registry will uniquely identify the view.
    if (app_list::AnswerCardContentsRegistry::Get()) {
      web_view_ = std::make_unique<views::WebView>(profile);
      web_view_->set_owned_by_client();
      web_view_->SetResizeBackgroundColor(SK_ColorTRANSPARENT);
      web_view_->SetWebContents(web_contents_.get());

      embed_token_ = app_list::AnswerCardContentsRegistry::Get()->Register(
          web_view_.get());

      std::move(callback).Run(embed_token_.value());
    }
    // TODO(dmblack): Handle Mash case.
  }

  AssistantCardRenderer* assistant_card_renderer_;

  std::unique_ptr<content::WebContents> web_contents_;
  std::unique_ptr<views::WebView> web_view_;
  base::Optional<base::UnguessableToken> embed_token_;

  DISALLOW_COPY_AND_ASSIGN(AssistantCard);
};

}  // namespace

AssistantCardRenderer::AssistantCardRenderer(
    service_manager::Connector* connector)
    : connector_(connector), binding_(this) {
  // Bind to the Assistant controller in ash.
  ash::mojom::AssistantControllerPtr assistant_controller;
  connector_->BindInterface(ash::mojom::kServiceName, &assistant_controller);
  ash::mojom::AssistantCardRendererPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  assistant_controller->SetAssistantCardRenderer(std::move(ptr));
}

AssistantCardRenderer::~AssistantCardRenderer() = default;

void AssistantCardRenderer::Render(
    const AccountId& account_id,
    const base::UnguessableToken& id_token,
    ash::mojom::AssistantCardParamsPtr params,
    ash::mojom::AssistantCardRenderer::RenderCallback callback) {
  DCHECK(assistant_cards_.count(id_token) == 0);
  assistant_cards_[id_token] = std::make_unique<AssistantCard>(
      this, account_id, std::move(params), std::move(callback));
}

void AssistantCardRenderer::Release(const base::UnguessableToken& id_token) {
  assistant_cards_.erase(id_token);
}

void AssistantCardRenderer::ReleaseAll(
    const std::vector<base::UnguessableToken>& id_tokens) {
  for (const base::UnguessableToken& id_token : id_tokens)
    assistant_cards_.erase(id_token);
}

void AssistantCardRenderer::OnCardPressed(const GURL& url) {
  ash::mojom::AssistantControllerPtr assistant_controller;
  connector_->BindInterface(ash::mojom::kServiceName, &assistant_controller);
  assistant_controller->OnCardPressed(url);
}
