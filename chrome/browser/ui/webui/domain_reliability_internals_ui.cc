// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/domain_reliability_internals_ui.h"

#include <string>

#include "chrome/browser/domain_reliability/service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "components/domain_reliability/service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

using domain_reliability::DomainReliabilityService;
using domain_reliability::DomainReliabilityServiceFactory;

DomainReliabilityInternalsUI::DomainReliabilityInternalsUI(
    content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  content::WebUIDataSource* html_source = content::WebUIDataSource::Create(
      chrome::kChromeUIDomainReliabilityInternalsHost);
  html_source->AddResourcePath("domain_reliability_internals.css",
      IDR_DOMAIN_RELIABILITY_INTERNALS_CSS);
  html_source->AddResourcePath("domain_reliability_internals.js",
      IDR_DOMAIN_RELIABILITY_INTERNALS_JS);
  html_source->SetDefaultResource(IDR_DOMAIN_RELIABILITY_INTERNALS_HTML);
  html_source->UseGzip();

  web_ui->RegisterMessageCallback(
      "updateData",
      base::BindRepeating(&DomainReliabilityInternalsUI::UpdateData,
                          base::Unretained(this)));

  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, html_source);
}

DomainReliabilityInternalsUI::~DomainReliabilityInternalsUI() {}

void DomainReliabilityInternalsUI::UpdateData(
    const base::ListValue* args) const {
  Profile* profile = Profile::FromWebUI(web_ui());
  DomainReliabilityServiceFactory* factory =
      DomainReliabilityServiceFactory::GetInstance();
  DCHECK(profile);
  DCHECK(factory);

  DomainReliabilityService* service = factory->GetForBrowserContext(profile);
  if (!service) {
    base::DictionaryValue* data = new base::DictionaryValue();
    data->SetString("error", "no_service");
    OnDataUpdated(std::unique_ptr<base::Value>(data));
    return;
  }

  service->GetWebUIData(base::Bind(
      &DomainReliabilityInternalsUI::OnDataUpdated,
      base::Unretained(this)));
}

void DomainReliabilityInternalsUI::OnDataUpdated(
    std::unique_ptr<base::Value> data) const {
  web_ui()->CallJavascriptFunctionUnsafe(
      "DomainReliabilityInternals.onDataUpdated", *data);
}
