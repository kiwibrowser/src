// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/resources_private/resources_private_api.h"

#include <string>
#include <utility>

#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/extensions/api/resources_private.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"

// To add a new component to this API, simply:
// 1. Add your component to the Component enum in
//      chrome/common/extensions/api/resources_private.idl
// 2. Create an AddStringsForMyComponent(base::DictionaryValue * dict) method.
// 3. Tie in that method to the switch statement in Run()

namespace extensions {

namespace {

void SetL10nString(base::DictionaryValue* dict, const std::string& string_id,
                   int resource_id) {
  dict->SetString(string_id, l10n_util::GetStringUTF16(resource_id));
}

void AddStringsForIdentity(base::DictionaryValue* dict) {
  SetL10nString(dict, "window-title", IDS_EXTENSION_CONFIRM_PERMISSIONS);
}

void AddStringsForPdf(base::DictionaryValue* dict) {
  SetL10nString(dict, "passwordDialogTitle", IDS_PDF_PASSWORD_DIALOG_TITLE);
  SetL10nString(dict, "passwordPrompt", IDS_PDF_NEED_PASSWORD);
  SetL10nString(dict, "passwordSubmit", IDS_PDF_PASSWORD_SUBMIT);
  SetL10nString(dict, "passwordInvalid", IDS_PDF_PASSWORD_INVALID);
  SetL10nString(dict, "pageLoading", IDS_PDF_PAGE_LOADING);
  SetL10nString(dict, "pageLoadFailed", IDS_PDF_PAGE_LOAD_FAILED);
  SetL10nString(dict, "errorDialogTitle", IDS_PDF_ERROR_DIALOG_TITLE);
  SetL10nString(dict, "pageReload", IDS_PDF_PAGE_RELOAD_BUTTON);
  SetL10nString(dict, "bookmarks", IDS_PDF_BOOKMARKS);
  SetL10nString(dict, "labelPageNumber", IDS_PDF_LABEL_PAGE_NUMBER);
  SetL10nString(dict, "tooltipRotateCW", IDS_PDF_TOOLTIP_ROTATE_CW);
  SetL10nString(dict, "tooltipDownload", IDS_PDF_TOOLTIP_DOWNLOAD);
  SetL10nString(dict, "tooltipPrint", IDS_PDF_TOOLTIP_PRINT);
  SetL10nString(dict, "tooltipFitToPage", IDS_PDF_TOOLTIP_FIT_PAGE);
  SetL10nString(dict, "tooltipFitToWidth", IDS_PDF_TOOLTIP_FIT_WIDTH);
  SetL10nString(dict, "tooltipZoomIn", IDS_PDF_TOOLTIP_ZOOM_IN);
  SetL10nString(dict, "tooltipZoomOut", IDS_PDF_TOOLTIP_ZOOM_OUT);
}

}  // namespace

namespace get_strings = api::resources_private::GetStrings;

ResourcesPrivateGetStringsFunction::ResourcesPrivateGetStringsFunction() {}

ResourcesPrivateGetStringsFunction::~ResourcesPrivateGetStringsFunction() {}

ExtensionFunction::ResponseAction ResourcesPrivateGetStringsFunction::Run() {
  std::unique_ptr<get_strings::Params> params(
      get_strings::Params::Create(*args_));
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);

  api::resources_private::Component component = params->component;

  switch (component) {
    case api::resources_private::COMPONENT_IDENTITY:
      AddStringsForIdentity(dict.get());
      break;
    case api::resources_private::COMPONENT_PDF:
      AddStringsForPdf(dict.get());
      break;
    case api::resources_private::COMPONENT_NONE:
      NOTREACHED();
  }

  const std::string& app_locale = g_browser_process->GetApplicationLocale();
  webui::SetLoadTimeDataDefaults(app_locale, dict.get());

  return RespondNow(OneArgument(std::move(dict)));
}

}  // namespace extensions
