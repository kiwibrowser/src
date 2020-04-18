// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/management/chrome_management_api_delegate.h"

#include <memory>

#include "base/callback_helpers.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/bookmark_app_helper.h"
#include "chrome/browser/extensions/chrome_extension_function_details.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/launch_util.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/ui/webui/extensions/extension_icon_source.h"
#include "chrome/common/extensions/extension_metrics.h"
#include "chrome/common/extensions/manifest_handlers/app_launch_info.h"
#include "chrome/common/web_application_info.h"
#include "components/favicon/core/favicon_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/service_manager_connection.h"
#include "extensions/browser/api/management/management_api.h"
#include "extensions/browser/api/management/management_api_constants.h"
#include "extensions/browser/disable_reason.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension.h"
#include "services/data_decoder/public/cpp/safe_json_parser.h"

namespace {

class ManagementSetEnabledFunctionInstallPromptDelegate
    : public extensions::InstallPromptDelegate {
 public:
  ManagementSetEnabledFunctionInstallPromptDelegate(
      content::WebContents* web_contents,
      content::BrowserContext* browser_context,
      const extensions::Extension* extension,
      const base::Callback<void(bool)>& callback)
      : install_prompt_(new ExtensionInstallPrompt(web_contents)),
        callback_(callback),
        weak_factory_(this) {
    ExtensionInstallPrompt::PromptType type =
        ExtensionInstallPrompt::GetReEnablePromptTypeForExtension(
            browser_context, extension);
    install_prompt_->ShowDialog(
        base::Bind(&ManagementSetEnabledFunctionInstallPromptDelegate::
                       OnInstallPromptDone,
                   weak_factory_.GetWeakPtr()),
        extension, nullptr,
        std::make_unique<ExtensionInstallPrompt::Prompt>(type),
        ExtensionInstallPrompt::GetDefaultShowDialogCallback());
  }
  ~ManagementSetEnabledFunctionInstallPromptDelegate() override {}

 private:
  void OnInstallPromptDone(ExtensionInstallPrompt::Result result) {
    base::ResetAndReturn(&callback_).Run(
        result == ExtensionInstallPrompt::Result::ACCEPTED);
  }

  // Used for prompting to re-enable items with permissions escalation updates.
  std::unique_ptr<ExtensionInstallPrompt> install_prompt_;

  base::Callback<void(bool)> callback_;

  base::WeakPtrFactory<ManagementSetEnabledFunctionInstallPromptDelegate>
      weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ManagementSetEnabledFunctionInstallPromptDelegate);
};

class ManagementUninstallFunctionUninstallDialogDelegate
    : public extensions::ExtensionUninstallDialog::Delegate,
      public extensions::UninstallDialogDelegate {
 public:
  ManagementUninstallFunctionUninstallDialogDelegate(
      extensions::ManagementUninstallFunctionBase* function,
      const extensions::Extension* target_extension,
      bool show_programmatic_uninstall_ui)
      : function_(function) {
    ChromeExtensionFunctionDetails details(function);
    extension_uninstall_dialog_.reset(
        extensions::ExtensionUninstallDialog::Create(
            details.GetProfile(), details.GetNativeWindowForUI(), this));
    bool uninstall_from_webstore =
        function->extension() &&
        function->extension()->id() == extensions::kWebStoreAppId;
    extensions::UninstallSource source;
    extensions::UninstallReason reason;
    if (uninstall_from_webstore) {
      source = extensions::UNINSTALL_SOURCE_CHROME_WEBSTORE;
      reason = extensions::UNINSTALL_REASON_CHROME_WEBSTORE;
    } else if (function->source_context_type() ==
               extensions::Feature::WEBUI_CONTEXT) {
      source = extensions::UNINSTALL_SOURCE_CHROME_EXTENSIONS_PAGE;
      // TODO: Update this to a new reason; it shouldn't be lumped in with
      // other uninstalls if it's from the chrome://extensions page.
      reason = extensions::UNINSTALL_REASON_MANAGEMENT_API;
    } else {
      source = extensions::UNINSTALL_SOURCE_EXTENSION;
      reason = extensions::UNINSTALL_REASON_MANAGEMENT_API;
    }
    if (show_programmatic_uninstall_ui) {
      extension_uninstall_dialog_->ConfirmUninstallByExtension(
          target_extension, function->extension(), reason, source);
    } else {
      extension_uninstall_dialog_->ConfirmUninstall(target_extension, reason,
                                                    source);
    }
  }

  ~ManagementUninstallFunctionUninstallDialogDelegate() override {}

  // ExtensionUninstallDialog::Delegate implementation.
  void OnExtensionUninstallDialogClosed(bool did_start_uninstall,
                                        const base::string16& error) override {
    function_->OnExtensionUninstallDialogClosed(did_start_uninstall, error);
  }

 private:
  extensions::ManagementUninstallFunctionBase* function_;
  std::unique_ptr<extensions::ExtensionUninstallDialog>
      extension_uninstall_dialog_;

  DISALLOW_COPY_AND_ASSIGN(ManagementUninstallFunctionUninstallDialogDelegate);
};

class ChromeAppForLinkDelegate : public extensions::AppForLinkDelegate {
 public:
  ChromeAppForLinkDelegate() {}
  ~ChromeAppForLinkDelegate() override {}

  void OnFaviconForApp(
      extensions::ManagementGenerateAppForLinkFunction* function,
      content::BrowserContext* context,
      const std::string& title,
      const GURL& launch_url,
      const favicon_base::FaviconImageResult& image_result) {
    WebApplicationInfo web_app;
    web_app.title = base::UTF8ToUTF16(title);
    web_app.app_url = launch_url;

    if (!image_result.image.IsEmpty()) {
      WebApplicationInfo::IconInfo icon;
      icon.data = image_result.image.AsBitmap();
      icon.width = icon.data.width();
      icon.height = icon.data.height();
      web_app.icons.push_back(icon);
    }

    bookmark_app_helper_.reset(new extensions::BookmarkAppHelper(
        Profile::FromBrowserContext(context), web_app, nullptr,
        WebappInstallSource::MANAGEMENT_API));
    bookmark_app_helper_->Create(
        base::Bind(&extensions::ManagementGenerateAppForLinkFunction::
                       FinishCreateBookmarkApp,
                   function));
  }

  std::unique_ptr<extensions::BookmarkAppHelper> bookmark_app_helper_;

  // Used for favicon loading tasks.
  base::CancelableTaskTracker cancelable_task_tracker_;
};

}  // namespace

ChromeManagementAPIDelegate::ChromeManagementAPIDelegate() {
}

ChromeManagementAPIDelegate::~ChromeManagementAPIDelegate() {
}

void ChromeManagementAPIDelegate::LaunchAppFunctionDelegate(
    const extensions::Extension* extension,
    content::BrowserContext* context) const {
  // Look at prefs to find the right launch container.
  // If the user has not set a preference, the default launch value will be
  // returned.
  extensions::LaunchContainer launch_container =
      GetLaunchContainer(extensions::ExtensionPrefs::Get(context), extension);
  OpenApplication(AppLaunchParams(Profile::FromBrowserContext(context),
                                  extension, launch_container,
                                  WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                  extensions::SOURCE_MANAGEMENT_API));
  extensions::RecordAppLaunchType(extension_misc::APP_LAUNCH_EXTENSION_API,
                                  extension->GetType());
}

GURL ChromeManagementAPIDelegate::GetFullLaunchURL(
    const extensions::Extension* extension) const {
  return extensions::AppLaunchInfo::GetFullLaunchURL(extension);
}

extensions::LaunchType ChromeManagementAPIDelegate::GetLaunchType(
    const extensions::ExtensionPrefs* prefs,
    const extensions::Extension* extension) const {
  return extensions::GetLaunchType(prefs, extension);
}

void ChromeManagementAPIDelegate::
    GetPermissionWarningsByManifestFunctionDelegate(
        extensions::ManagementGetPermissionWarningsByManifestFunction* function,
        const std::string& manifest_str) const {
  data_decoder::SafeJsonParser::Parse(
      content::ServiceManagerConnection::GetForProcess()->GetConnector(),
      manifest_str,
      base::Bind(
          &extensions::ManagementGetPermissionWarningsByManifestFunction::
              OnParseSuccess,
          function),
      base::Bind(
          &extensions::ManagementGetPermissionWarningsByManifestFunction::
              OnParseFailure,
          function));
}

std::unique_ptr<extensions::InstallPromptDelegate>
ChromeManagementAPIDelegate::SetEnabledFunctionDelegate(
    content::WebContents* web_contents,
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    const base::Callback<void(bool)>& callback) const {
  return std::unique_ptr<ManagementSetEnabledFunctionInstallPromptDelegate>(
      new ManagementSetEnabledFunctionInstallPromptDelegate(
          web_contents, browser_context, extension, callback));
}

std::unique_ptr<extensions::UninstallDialogDelegate>
ChromeManagementAPIDelegate::UninstallFunctionDelegate(
    extensions::ManagementUninstallFunctionBase* function,
    const extensions::Extension* target_extension,
    bool show_programmatic_uninstall_ui) const {
  return std::unique_ptr<extensions::UninstallDialogDelegate>(
      new ManagementUninstallFunctionUninstallDialogDelegate(
          function, target_extension, show_programmatic_uninstall_ui));
}

bool ChromeManagementAPIDelegate::CreateAppShortcutFunctionDelegate(
    extensions::ManagementCreateAppShortcutFunction* function,
    const extensions::Extension* extension,
    std::string* error) const {
  Browser* browser = chrome::FindBrowserWithProfile(
      Profile::FromBrowserContext(function->browser_context()));
  if (!browser) {
    // Shouldn't happen if we have user gesture.
    *error = extension_management_api_constants::kNoBrowserToCreateShortcut;
    return false;
  }

  chrome::ShowCreateChromeAppShortcutsDialog(
      browser->window()->GetNativeWindow(), browser->profile(), extension,
      base::Bind(&extensions::ManagementCreateAppShortcutFunction::
                     OnCloseShortcutPrompt,
                 function));

  return true;
}

std::unique_ptr<extensions::AppForLinkDelegate>
ChromeManagementAPIDelegate::GenerateAppForLinkFunctionDelegate(
    extensions::ManagementGenerateAppForLinkFunction* function,
    content::BrowserContext* context,
    const std::string& title,
    const GURL& launch_url) const {
  favicon::FaviconService* favicon_service =
      FaviconServiceFactory::GetForProfile(Profile::FromBrowserContext(context),
                                           ServiceAccessType::EXPLICIT_ACCESS);
  DCHECK(favicon_service);

  ChromeAppForLinkDelegate* delegate = new ChromeAppForLinkDelegate;

  favicon_service->GetFaviconImageForPageURL(
      launch_url,
      base::Bind(&ChromeAppForLinkDelegate::OnFaviconForApp,
                 base::Unretained(delegate), base::RetainedRef(function),
                 context, title, launch_url),
      &delegate->cancelable_task_tracker_);

  return std::unique_ptr<extensions::AppForLinkDelegate>(delegate);
}

bool ChromeManagementAPIDelegate::CanHostedAppsOpenInWindows() const {
  return extensions::util::CanHostedAppsOpenInWindows();
}

bool ChromeManagementAPIDelegate::IsNewBookmarkAppsEnabled() const {
  return extensions::util::IsNewBookmarkAppsEnabled();
}

void ChromeManagementAPIDelegate::EnableExtension(
    content::BrowserContext* context,
    const std::string& extension_id) const {
  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(context)->GetExtensionById(
          extension_id, extensions::ExtensionRegistry::EVERYTHING);
  // If the extension was disabled for a permissions increase, the Management
  // API will have displayed a re-enable prompt to the user, so we know it's
  // safe to grant permissions here.
  extensions::ExtensionSystem::Get(context)
      ->extension_service()
      ->GrantPermissionsAndEnableExtension(extension);
}

void ChromeManagementAPIDelegate::DisableExtension(
    content::BrowserContext* context,
    const extensions::Extension* source_extension,
    const std::string& extension_id,
    extensions::disable_reason::DisableReason disable_reason) const {
  extensions::ExtensionSystem::Get(context)
      ->extension_service()
      ->DisableExtensionWithSource(source_extension, extension_id,
                                   disable_reason);
}

bool ChromeManagementAPIDelegate::UninstallExtension(
    content::BrowserContext* context,
    const std::string& transient_extension_id,
    extensions::UninstallReason reason,
    base::string16* error) const {
  return extensions::ExtensionSystem::Get(context)
      ->extension_service()
      ->UninstallExtension(transient_extension_id, reason, error);
}

void ChromeManagementAPIDelegate::SetLaunchType(
    content::BrowserContext* context,
    const std::string& extension_id,
    extensions::LaunchType launch_type) const {
  extensions::SetLaunchType(context, extension_id, launch_type);
}

GURL ChromeManagementAPIDelegate::GetIconURL(
    const extensions::Extension* extension,
    int icon_size,
    ExtensionIconSet::MatchType match,
    bool grayscale) const {
  return extensions::ExtensionIconSource::GetIconURL(extension, icon_size,
                                                     match, grayscale);
}
