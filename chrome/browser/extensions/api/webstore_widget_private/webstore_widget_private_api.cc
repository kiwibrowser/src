// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/webstore_widget_private/webstore_widget_private_api.h"

#include <memory>
#include <utility>

#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/file_manager/app_id.h"
#include "chrome/browser/extensions/api/webstore_widget_private/app_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/webstore_widget_private.h"
#include "chrome/grit/generated_resources.h"
#include "extensions/browser/extension_function_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"

namespace extensions {
namespace api {

namespace {

const char kGoogleCastApiExtensionId[] = "mafeflapfdfljijmlienjedomfjfmhpd";

void SetL10nString(base::DictionaryValue* dict, const std::string& string_id,
                   int resource_id) {
  dict->SetString(string_id, l10n_util::GetStringUTF16(resource_id));
}

}  // namespace

WebstoreWidgetPrivateGetStringsFunction::
    WebstoreWidgetPrivateGetStringsFunction() {
}

WebstoreWidgetPrivateGetStringsFunction::
    ~WebstoreWidgetPrivateGetStringsFunction() {
}

ExtensionFunction::ResponseAction
WebstoreWidgetPrivateGetStringsFunction::Run() {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());

  SetL10nString(dict.get(), "TITLE_PRINTER_PROVIDERS",
                IDS_WEBSTORE_WIDGET_TITLE_PRINTER_PROVIDERS);
  SetL10nString(dict.get(), "DEFAULT_ERROR_MESSAGE",
                IDS_WEBSTORE_WIDGET_DEFAULT_ERROR);
  SetL10nString(dict.get(), "OK_BUTTON", IDS_FILE_BROWSER_OK_LABEL);
  SetL10nString(dict.get(), "INSTALLATION_FAILED_MESSAGE",
                IDS_FILE_BROWSER_SUGGEST_DIALOG_INSTALLATION_FAILED);
  SetL10nString(dict.get(), "LINK_TO_WEBSTORE",
                IDS_FILE_BROWSER_SUGGEST_DIALOG_LINK_TO_WEBSTORE);
  SetL10nString(dict.get(), "LOADING_SPINNER_ALT",
                IDS_WEBSTORE_WIDGET_LOADING_SPINNER_ALT);
  SetL10nString(dict.get(), "INSTALLING_SPINNER_ALT",
                IDS_WEBSTORE_WIDGET_INSTALLING_SPINNER_ALT);

  const std::string& app_locale = g_browser_process->GetApplicationLocale();
  webui::SetLoadTimeDataDefaults(app_locale, dict.get());
  return RespondNow(OneArgument(std::move(dict)));
}

WebstoreWidgetPrivateInstallWebstoreItemFunction::
    WebstoreWidgetPrivateInstallWebstoreItemFunction() {
}

WebstoreWidgetPrivateInstallWebstoreItemFunction::
    ~WebstoreWidgetPrivateInstallWebstoreItemFunction() {
}

ExtensionFunction::ResponseAction
WebstoreWidgetPrivateInstallWebstoreItemFunction::Run() {
  const std::unique_ptr<webstore_widget_private::InstallWebstoreItem::Params>
      params(
          webstore_widget_private::InstallWebstoreItem::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  if (params->item_id.empty())
    return RespondNow(Error("App ID empty."));

  bool allow_silent_install =
      extension()->id() == file_manager::kVideoPlayerAppId &&
      params->item_id == kGoogleCastApiExtensionId;
  if (params->silent_installation && !allow_silent_install)
    return RespondNow(Error("Silent installation not allowed."));

  const extensions::WebstoreStandaloneInstaller::Callback callback = base::Bind(
      &WebstoreWidgetPrivateInstallWebstoreItemFunction::OnInstallComplete,
      this);

  content::WebContents* web_contents = GetSenderWebContents();
  if (!web_contents) {
    return RespondNow(
        Error(function_constants::kCouldNotFindSenderWebContents));
  }
  scoped_refptr<webstore_widget::AppInstaller> installer(
      new webstore_widget::AppInstaller(
          web_contents, params->item_id,
          Profile::FromBrowserContext(browser_context()),
          params->silent_installation, callback));
  // installer will be AddRef()'d in BeginInstall().
  installer->BeginInstall();

  return RespondLater();
}

void WebstoreWidgetPrivateInstallWebstoreItemFunction::OnInstallComplete(
    bool success,
    const std::string& error,
    extensions::webstore_install::Result result) {
  Respond(success ? NoArguments() : Error(error));
}

}  // namespace api
}  // namespace extensions
