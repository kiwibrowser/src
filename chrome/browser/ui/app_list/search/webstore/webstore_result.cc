// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/webstore/webstore_result.h"

#include <stddef.h>

#include <algorithm>
#include <vector>

#include "ash/public/cpp/app_list/app_list_constants.h"
#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/install_tracker.h"
#include "chrome/browser/extensions/install_tracker_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/search/common/url_icon_source.h"
#include "chrome/browser/ui/app_list/search/search_util.h"
#include "chrome/browser/ui/app_list/search/webstore/webstore_installer.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension_urls.h"
#include "net/base/url_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace app_list {

WebstoreResult::WebstoreResult(Profile* profile,
                               const std::string& app_id,
                               const GURL& icon_url,
                               bool is_paid,
                               extensions::Manifest::Type item_type,
                               AppListControllerDelegate* controller)
    : profile_(profile),
      app_id_(app_id),
      icon_url_(icon_url),
      is_paid_(is_paid),
      item_type_(item_type),
      controller_(controller),
      install_tracker_(NULL),
      extension_registry_(NULL),
      weak_factory_(this) {
  set_id(GetResultIdFromExtensionId(app_id));
  SetResultType(ash::SearchResultType::kWebStoreSearch);
  SetDefaultDetails();

  InitAndStartObserving();
  UpdateActions();

  int icon_dimension = GetPreferredIconDimension(display_type());
  icon_ = gfx::ImageSkia(
      std::make_unique<UrlIconSource>(
          base::Bind(&WebstoreResult::OnIconLoaded, weak_factory_.GetWeakPtr()),
          profile_, icon_url_, icon_dimension, IDR_WEBSTORE_ICON_32),
      gfx::Size(icon_dimension, icon_dimension));
  SetIcon(icon_);
}

WebstoreResult::~WebstoreResult() {
  StopObservingInstall();
  StopObservingRegistry();
}

// static
std::string WebstoreResult::GetResultIdFromExtensionId(
    const std::string& extension_id) {
  return extension_urls::GetWebstoreItemDetailURLPrefix() + extension_id;
}

void WebstoreResult::Open(int event_flags) {
  RecordHistogram(SEARCH_WEBSTORE_SEARCH_RESULT);
  const GURL store_url = net::AppendQueryParameter(
      GURL(extension_urls::GetWebstoreItemDetailURLPrefix() + app_id_),
      extension_urls::kWebstoreSourceField,
      extension_urls::kLaunchSourceAppListSearch);

  controller_->OpenURL(profile_,
                       store_url,
                       ui::PAGE_TRANSITION_LINK,
                       ui::DispositionFromEventFlags(event_flags));
}

void WebstoreResult::InvokeAction(int action_index, int event_flags) {
  if (is_paid_) {
    // Paid apps cannot be installed directly from the launcher. Instead, open
    // the webstore page for the app.
    Open(event_flags);
    return;
  }

  StartInstall();
}

void WebstoreResult::InitAndStartObserving() {
  DCHECK(!install_tracker_ && !extension_registry_);

  install_tracker_ =
      extensions::InstallTrackerFactory::GetForBrowserContext(profile_);
  extension_registry_ = extensions::ExtensionRegistry::Get(profile_);

  const extensions::ActiveInstallData* install_data =
      install_tracker_->GetActiveInstall(app_id_);
  if (install_data) {
    SetPercentDownloaded(install_data->percent_downloaded);
    SetIsInstalling(true);
  }

  install_tracker_->AddObserver(this);
  extension_registry_->AddObserver(this);
}

void WebstoreResult::UpdateActions() {
  Actions actions;

  const bool is_otr = profile_->IsOffTheRecord();
  const bool is_installed =
      extension_registry_->GetExtensionById(
          app_id_, extensions::ExtensionRegistry::EVERYTHING) != nullptr;

  if (!is_otr && !is_installed && !is_installing()) {
    actions.push_back(Action(
        l10n_util::GetStringUTF16(IDS_EXTENSION_INLINE_INSTALL_PROMPT_TITLE),
        base::string16()));
  }

  SetActions(actions);
}

void WebstoreResult::SetDefaultDetails() {
  const base::string16 details =
      l10n_util::GetStringUTF16(IDS_EXTENSION_WEB_STORE_TITLE);
  Tags details_tags;
  details_tags.push_back(Tag(ash::SearchResultTag::DIM, 0, details.length()));

  SetDetails(details);
  SetDetailsTags(details_tags);
}

void WebstoreResult::OnIconLoaded() {
  // Set webstore badge.
  SetBadgeIcon(*ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
      IDR_WEBSTORE_ICON_16));

  // Remove the existing image reps since the icon data is loaded and they
  // need to be re-created.
  const std::vector<gfx::ImageSkiaRep>& image_reps = icon_.image_reps();
  for (size_t i = 0; i < image_reps.size(); ++i)
    icon_.RemoveRepresentation(image_reps[i].scale());

  SetIcon(icon_);
}

void WebstoreResult::StartInstall() {
  SetPercentDownloaded(0);
  SetIsInstalling(true);

  scoped_refptr<WebstoreInstaller> installer = new WebstoreInstaller(
      app_id_, profile_,
      base::Bind(&WebstoreResult::InstallCallback, weak_factory_.GetWeakPtr()));
  installer->BeginInstall();
}

void WebstoreResult::InstallCallback(
    bool success,
    const std::string& error,
    extensions::webstore_install::Result result) {
  if (!success) {
    LOG(ERROR) << "Failed to install app, error=" << error;
    SetIsInstalling(false);
    return;
  }

  // Success handling is continued in OnExtensionInstalled.
  SetPercentDownloaded(100);
}

void WebstoreResult::LaunchCallback(extensions::webstore_install::Result result,
                                    const std::string& error) {
  if (result != extensions::webstore_install::SUCCESS)
    LOG(ERROR) << "Failed to launch app, error=" << error;

  SetIsInstalling(false);
}

void WebstoreResult::StopObservingInstall() {
  if (install_tracker_)
    install_tracker_->RemoveObserver(this);
  install_tracker_ = NULL;
}

void WebstoreResult::StopObservingRegistry() {
  if (extension_registry_)
    extension_registry_->RemoveObserver(this);
  extension_registry_ = NULL;
}

void WebstoreResult::OnDownloadProgress(const std::string& extension_id,
                                        int percent_downloaded) {
  if (extension_id != app_id_ || percent_downloaded < 0)
    return;

  SetPercentDownloaded(percent_downloaded);
}

void WebstoreResult::OnExtensionInstalled(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    bool is_update) {
  if (extension->id() != app_id_)
    return;

  SetIsInstalling(false);
  UpdateActions();

  if (extension_registry_->GetExtensionById(
          app_id_, extensions::ExtensionRegistry::EVERYTHING)) {
    NotifyItemInstalled();
  }
}

void WebstoreResult::OnShutdown() {
  StopObservingInstall();
}

void WebstoreResult::OnShutdown(extensions::ExtensionRegistry* registry) {
  StopObservingRegistry();
}

}  // namespace app_list
