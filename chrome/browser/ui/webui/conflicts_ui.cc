// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/conflicts_ui.h"

#include <memory>

#include "base/memory/ref_counted_memory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/conflicts_handler.h"
#include "chrome/browser/ui/webui/module_database_conflicts_handler.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

content::WebUIDataSource* CreateConflictsUIHTMLSource() {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIConflictsHost);

  source->AddLocalizedString("loadingMessage", IDS_CONFLICTS_LOADING_MESSAGE);
  source->AddLocalizedString("modulesLongTitle",
                             IDS_CONFLICTS_CHECK_PAGE_TITLE_LONG);
  source->AddLocalizedString("modulesBlurb", IDS_CONFLICTS_EXPLANATION_TEXT);
  source->AddLocalizedString("moduleSuspectedBad",
                             IDS_CONFLICTS_CHECK_WARNING_SUSPECTED);
  source->AddLocalizedString("moduleConfirmedBad",
                     IDS_CONFLICTS_CHECK_WARNING_CONFIRMED);
  source->AddLocalizedString("helpCenterLink", IDS_LEARN_MORE);
  source->AddLocalizedString("investigatingText",
                             IDS_CONFLICTS_CHECK_INVESTIGATING);
  source->AddLocalizedString("modulesNoneLoaded",
                             IDS_CONFLICTS_NO_MODULES_LOADED);
  source->AddLocalizedString("headerSoftware", IDS_CONFLICTS_HEADER_SOFTWARE);
  source->AddLocalizedString("headerSignedBy", IDS_CONFLICTS_HEADER_SIGNED_BY);
  source->AddLocalizedString("headerLocation", IDS_CONFLICTS_HEADER_LOCATION);
  source->AddLocalizedString("headerVersion", IDS_CONFLICTS_HEADER_VERSION);
  source->AddLocalizedString("headerHelpTip", IDS_CONFLICTS_HEADER_HELP_TIP);
  source->SetJsonPath("strings.js");
  source->AddResourcePath("conflicts.js", IDR_ABOUT_CONFLICTS_JS);
  source->SetDefaultResource(IDR_ABOUT_CONFLICTS_HTML);
  return source;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
//
// ConflictsUI
//
///////////////////////////////////////////////////////////////////////////////

ConflictsUI::ConflictsUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  if (base::FeatureList::IsEnabled(features::kModuleDatabase)) {
    web_ui->AddMessageHandler(
        std::make_unique<ModuleDatabaseConflictsHandler>());
  } else {
    web_ui->AddMessageHandler(std::make_unique<ConflictsHandler>());
  }

  // Set up the about:conflicts source.
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, CreateConflictsUIHTMLSource());
}

// static
base::RefCountedMemory* ConflictsUI::GetFaviconResourceBytes(
      ui::ScaleFactor scale_factor) {
  return static_cast<base::RefCountedMemory*>(
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytesForScale(
          IDR_CONFLICT_FAVICON, scale_factor));
}
