// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/drive_app_registry.h"

#include <stddef.h>

#include <algorithm>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "components/drive/drive_app_registry_observer.h"
#include "components/drive/service/drive_service_interface.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/google_api_keys.h"

namespace {

// Add {selector -> app_id} mapping to |map|.
void AddAppSelectorList(
    const std::vector<std::unique_ptr<std::string>>& selectors,
    const std::string& app_id,
    std::multimap<std::string, std::string>* map) {
  for (size_t i = 0; i < selectors.size(); ++i)
    map->insert(std::make_pair(*selectors[i], app_id));
}

// Append list of app ids in |map| looked up by |selector| to |matched_apps|.
void FindAppsForSelector(const std::string& selector,
                         const std::multimap<std::string, std::string>& map,
                         std::vector<std::string>* matched_apps) {
  typedef std::multimap<std::string, std::string>::const_iterator iterator;
  std::pair<iterator, iterator> range = map.equal_range(selector);
  for (iterator it = range.first; it != range.second; ++it)
    matched_apps->push_back(it->second);
}

void RemoveAppFromSelector(const std::string& app_id,
                           std::multimap<std::string, std::string>* map) {
  typedef std::multimap<std::string, std::string>::iterator iterator;
  for (iterator it = map->begin(); it != map->end(); ) {
    iterator now = it++;
    if (now->second == app_id)
      map->erase(now);
  }
}

}  // namespace

namespace drive {

DriveAppInfo::DriveAppInfo() {
}

DriveAppInfo::DriveAppInfo(
    const std::string& app_id,
    const std::string& product_id,
    const IconList& app_icons,
    const IconList& document_icons,
    const std::string& app_name,
    const GURL& create_url,
    bool is_removable)
    : app_id(app_id),
      product_id(product_id),
      app_icons(app_icons),
      document_icons(document_icons),
      app_name(app_name),
      create_url(create_url),
      is_removable(is_removable) {
}

DriveAppInfo::DriveAppInfo(const DriveAppInfo& other) = default;

DriveAppInfo::~DriveAppInfo() {
}

DriveAppRegistry::DriveAppRegistry(DriveServiceInterface* drive_service)
    : drive_service_(drive_service),
      is_updating_(false),
      weak_ptr_factory_(this) {
}

DriveAppRegistry::~DriveAppRegistry() {
}

void DriveAppRegistry::GetAppsForFile(
    const base::FilePath::StringType& file_extension,
    const std::string& mime_type,
    std::vector<DriveAppInfo>* apps) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  std::vector<std::string> matched_apps;
  if (!file_extension.empty()) {
    const std::string without_dot =
        base::FilePath(file_extension.substr(1)).AsUTF8Unsafe();
    FindAppsForSelector(without_dot, extension_map_, &matched_apps);
  }
  if (!mime_type.empty())
    FindAppsForSelector(mime_type, mimetype_map_, &matched_apps);

  // Insert found Drive apps into |apps|, but skip duplicate results.
  std::set<std::string> inserted_app_ids;
  for (size_t i = 0; i < matched_apps.size(); ++i) {
    if (inserted_app_ids.count(matched_apps[i]) == 0) {
      inserted_app_ids.insert(matched_apps[i]);
      std::map<std::string, DriveAppInfo>::const_iterator it =
          all_apps_.find(matched_apps[i]);
      DCHECK(it != all_apps_.end());
      apps->push_back(it->second);
    }
  }
}

void DriveAppRegistry::GetAppList(std::vector<DriveAppInfo>* apps) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  apps->clear();
  for (std::map<std::string, DriveAppInfo>::const_iterator
          it = all_apps_.begin(); it != all_apps_.end(); ++it) {
    apps->push_back(it->second);
  }
}

void DriveAppRegistry::Update() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (is_updating_)  // There is already an update in progress.
    return;
  is_updating_ = true;

  drive_service_->GetAppList(
      base::Bind(&DriveAppRegistry::UpdateAfterGetAppList,
                 weak_ptr_factory_.GetWeakPtr()));
}

void DriveAppRegistry::UpdateAfterGetAppList(
    google_apis::DriveApiErrorCode gdata_error,
    std::unique_ptr<google_apis::AppList> app_list) {
  DCHECK(thread_checker_.CalledOnValidThread());

  DCHECK(is_updating_);
  is_updating_ = false;

  // Failed to fetch the data from the server. We can do nothing here.
  if (gdata_error != google_apis::HTTP_SUCCESS)
    return;

  DCHECK(app_list);
  UpdateFromAppList(*app_list);
}

void DriveAppRegistry::UpdateFromAppList(const google_apis::AppList& app_list) {
  all_apps_.clear();
  extension_map_.clear();
  mimetype_map_.clear();

  for (size_t i = 0; i < app_list.items().size(); ++i) {
    const google_apis::AppResource& app = *app_list.items()[i];
    const std::string id = app.application_id();

    DriveAppInfo::IconList app_icons;
    DriveAppInfo::IconList document_icons;
    for (size_t j = 0; j < app.icons().size(); ++j) {
      const google_apis::DriveAppIcon& icon = *app.icons()[j];
      if (icon.icon_url().is_empty())
        continue;
      if (icon.category() == google_apis::DriveAppIcon::APPLICATION)
        app_icons.push_back(std::make_pair(icon.icon_side_length(),
                                           icon.icon_url()));
      if (icon.category() == google_apis::DriveAppIcon::DOCUMENT)
        document_icons.push_back(std::make_pair(icon.icon_side_length(),
                                                icon.icon_url()));
    }

    all_apps_[id] = DriveAppInfo(app.application_id(),
                                 app.product_id(),
                                 app_icons,
                                 document_icons,
                                 app.name(),
                                 app.create_url(),
                                 app.is_removable());

    // TODO(kinaba): consider taking primary/secondary distinction into account.
    AddAppSelectorList(app.primary_mimetypes(), id, &mimetype_map_);
    AddAppSelectorList(app.secondary_mimetypes(), id, &mimetype_map_);
    AddAppSelectorList(app.primary_file_extensions(), id, &extension_map_);
    AddAppSelectorList(app.secondary_file_extensions(), id, &extension_map_);
  }

  for (auto& observer : observers_)
    observer.OnDriveAppRegistryUpdated();
}

void DriveAppRegistry::AddObserver(DriveAppRegistryObserver* observer) {
  observers_.AddObserver(observer);
}

void DriveAppRegistry::RemoveObserver(DriveAppRegistryObserver* observer) {
  observers_.RemoveObserver(observer);
}

void DriveAppRegistry::UninstallApp(const std::string& app_id,
                                    const UninstallCallback& callback) {
  DCHECK(!callback.is_null());

  drive_service_->UninstallApp(app_id,
                               base::Bind(&DriveAppRegistry::OnAppUninstalled,
                                          weak_ptr_factory_.GetWeakPtr(),
                                          app_id,
                                          callback));
}

void DriveAppRegistry::OnAppUninstalled(const std::string& app_id,
                                        const UninstallCallback& callback,
                                        google_apis::DriveApiErrorCode error) {
  if (error == google_apis::HTTP_NO_CONTENT) {
    all_apps_.erase(app_id);
    RemoveAppFromSelector(app_id, &mimetype_map_);
    RemoveAppFromSelector(app_id, &extension_map_);
  }
  callback.Run(error);
}

// static
bool DriveAppRegistry::IsAppUninstallSupported() {
  return google_apis::IsGoogleChromeAPIKeyUsed();
}

namespace util {

GURL FindPreferredIcon(const DriveAppInfo::IconList& icons,
                       int preferred_size) {
  if (icons.empty())
    return GURL();

  DriveAppInfo::IconList sorted_icons = icons;
  std::sort(sorted_icons.rbegin(), sorted_icons.rend());

  // Go forward while the size is larger or equal to preferred_size.
  size_t i = 1;
  while (i < sorted_icons.size() && sorted_icons[i].first >= preferred_size)
    ++i;
  return sorted_icons[i - 1].second;
}

}  // namespace util
}  // namespace drive
