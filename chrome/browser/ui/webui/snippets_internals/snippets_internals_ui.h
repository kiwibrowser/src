// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SNIPPETS_INTERNALS_SNIPPETS_INTERNALS_UI_H_
#define CHROME_BROWSER_UI_WEBUI_SNIPPETS_INTERNALS_SNIPPETS_INTERNALS_UI_H_

#include "base/macros.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/snippets_internals/snippets_internals.mojom.h"
#include "components/ntp_snippets/content_suggestions_service.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/webui/mojo_web_ui_controller.h"

class SnippetsInternalsPageHandler;

// The implementation for the chrome://snippets-internals page.
class SnippetsInternalsUI
    : public snippets_internals::mojom::PageHandlerFactory,
      public ui::MojoWebUIController {
 public:
  explicit SnippetsInternalsUI(content::WebUI* web_ui);
  ~SnippetsInternalsUI() override;

  void CreatePageHandler(snippets_internals::mojom::PagePtr page,
                         CreatePageHandlerCallback callback) override;

 private:
  void BindSnippetsInternalsPageHandlerFactory(
      snippets_internals::mojom::PageHandlerFactoryRequest request);

  std::unique_ptr<SnippetsInternalsPageHandler> page_handler_;
  ntp_snippets::ContentSuggestionsService* content_suggestions_service_;
  PrefService* pref_service_;

  // Binding from the mojo interface to concrete impl.
  mojo::Binding<snippets_internals::mojom::PageHandlerFactory> binding_;

  DISALLOW_COPY_AND_ASSIGN(SnippetsInternalsUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SNIPPETS_INTERNALS_SNIPPETS_INTERNALS_UI_H_
