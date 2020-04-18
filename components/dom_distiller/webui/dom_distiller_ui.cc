// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/webui/dom_distiller_ui.h"

#include <memory>

#include "components/dom_distiller/core/dom_distiller_constants.h"
#include "components/dom_distiller/core/dom_distiller_service.h"
#include "components/dom_distiller/webui/dom_distiller_handler.h"
#include "components/grit/components_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

namespace dom_distiller {

DomDistillerUi::DomDistillerUi(content::WebUI* web_ui,
                               DomDistillerService* service,
                               const std::string& scheme)
    : content::WebUIController(web_ui) {
  // Set up WebUIDataSource.
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(kChromeUIDomDistillerHost);
  source->SetDefaultResource(IDR_ABOUT_DOM_DISTILLER_HTML);
  source->AddResourcePath("about_dom_distiller.css",
                          IDR_ABOUT_DOM_DISTILLER_CSS);
  source->AddResourcePath("about_dom_distiller.js", IDR_ABOUT_DOM_DISTILLER_JS);

  source->AddLocalizedString("domDistillerTitle",
                             IDS_DOM_DISTILLER_WEBUI_TITLE);
  source->AddLocalizedString("addArticleUrl",
                             IDS_DOM_DISTILLER_WEBUI_ENTRY_URL);
  source->AddLocalizedString("addArticleAddButtonLabel",
                             IDS_DOM_DISTILLER_WEBUI_ENTRY_ADD);
  source->AddLocalizedString("addArticleFailedLabel",
                             IDS_DOM_DISTILLER_WEBUI_ENTRY_ADD_FAILED);
  source->AddLocalizedString("viewUrlButtonLabel",
                             IDS_DOM_DISTILLER_WEBUI_VIEW_URL);
  source->AddLocalizedString("viewUrlFailedLabel",
                             IDS_DOM_DISTILLER_WEBUI_VIEW_URL_FAILED);
  source->AddLocalizedString("loadingEntries",
                             IDS_DOM_DISTILLER_WEBUI_FETCHING_ENTRIES);
  source->AddLocalizedString("refreshButtonLabel",
                             IDS_DOM_DISTILLER_WEBUI_REFRESH);

  content::BrowserContext* browser_context =
      web_ui->GetWebContents()->GetBrowserContext();
  content::WebUIDataSource::Add(browser_context, source);
  source->SetJsonPath("strings.js");

  // Add message handler.
  web_ui->AddMessageHandler(
      std::make_unique<DomDistillerHandler>(service, scheme));
}

DomDistillerUi::~DomDistillerUi() {}

}  // namespace dom_distiller
