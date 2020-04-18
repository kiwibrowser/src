// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/snippets_internals/snippets_internals_ui.h"

#include "chrome/browser/ntp_snippets/content_suggestions_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/snippets_internals/snippets_internals_page_handler.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "content/public/browser/web_ui_data_source.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/chrome_feature_list.h"
#endif

SnippetsInternalsUI::SnippetsInternalsUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui), binding_(this) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUISnippetsInternalsHost);
  source->AddResourcePath("snippets_internals.css", IDR_SNIPPETS_INTERNALS_CSS);
  source->AddResourcePath("snippets_internals.js", IDR_SNIPPETS_INTERNALS_JS);
  source->AddResourcePath("snippets_internals.mojom.js",
                          IDR_SNIPPETS_INTERNALS_MOJO_JS);
  source->SetDefaultResource(IDR_SNIPPETS_INTERNALS_HTML);
  source->UseGzip();

    Profile* profile = Profile::FromWebUI(web_ui);
  content_suggestions_service_ =
      ContentSuggestionsServiceFactory::GetInstance()->GetForProfile(profile);
  pref_service_ = profile->GetPrefs();
  content::WebUIDataSource::Add(profile, source);
  AddHandlerToRegistry(base::BindRepeating(
      &SnippetsInternalsUI::BindSnippetsInternalsPageHandlerFactory,
      base::Unretained(this)));
}

SnippetsInternalsUI::~SnippetsInternalsUI() {}

void SnippetsInternalsUI::BindSnippetsInternalsPageHandlerFactory(
    snippets_internals::mojom::PageHandlerFactoryRequest request) {
  if (binding_.is_bound())
    binding_.Unbind();

  binding_.Bind(std::move(request));
}

void SnippetsInternalsUI::CreatePageHandler(
    snippets_internals::mojom::PagePtr page,
    CreatePageHandlerCallback callback) {
  DCHECK(page);
  snippets_internals::mojom::PageHandlerPtr handler;
  page_handler_.reset(new SnippetsInternalsPageHandler(
      mojo::MakeRequest(&handler), std::move(page),
      content_suggestions_service_, pref_service_));

  std::move(callback).Run(std::move(handler));
}
