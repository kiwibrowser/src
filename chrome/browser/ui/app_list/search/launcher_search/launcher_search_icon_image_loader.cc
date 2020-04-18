// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/launcher_search/launcher_search_icon_image_loader.h"

#include <utility>

#include "base/strings/string_util.h"
#include "chrome/browser/chromeos/launcher_search_provider/error_reporter.h"
#include "extensions/common/constants.h"
#include "skia/ext/image_operations.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace {

const int kTruncatedIconUrlMaxSize = 100;
const char kWarningMessagePrefix[] =
    "[chrome.launcherSearchProvider.setSearchResults]";

}  // namespace

namespace app_list {

LauncherSearchIconImageLoader::LauncherSearchIconImageLoader(
    const GURL& icon_url,
    Profile* profile,
    const extensions::Extension* extension,
    const int icon_dimension,
    std::unique_ptr<chromeos::launcher_search_provider::ErrorReporter>
        error_reporter)
    : profile_(profile),
      extension_(extension),
      icon_url_(icon_url),
      icon_size_(icon_dimension, icon_dimension),
      error_reporter_(std::move(error_reporter)) {}

LauncherSearchIconImageLoader::~LauncherSearchIconImageLoader() = default;

void LauncherSearchIconImageLoader::LoadResources() {
  DCHECK(custom_icon_image_.isNull());

  // Loads extension icon image and set it as main icon image.
  extension_icon_image_ = LoadExtensionIcon();
  CHECK(!extension_icon_image_.isNull());
  NotifyObserversIconImageChange();

  // If valid icon_url is provided as chrome-extension scheme with the host of
  // |extension|, load custom icon.
  if (icon_url_.is_empty()) {
    return;
  }

  if (!icon_url_.is_valid() ||
      !icon_url_.SchemeIs(extensions::kExtensionScheme) ||
      icon_url_.host() != extension_->id()) {
    std::vector<std::string> params;
    params.push_back(kWarningMessagePrefix);
    params.push_back(GetTruncatedIconUrl(kTruncatedIconUrlMaxSize));
    params.push_back(extensions::kExtensionScheme);
    params.push_back(extension_->id());
    error_reporter_->Warn(base::ReplaceStringPlaceholders(
        "$1 Invalid icon URL: $2. Must have a valid URL within $3://$4.",
        params, nullptr));
    return;
  }

  LoadIconResourceFromExtension();
}

void LauncherSearchIconImageLoader::AddObserver(Observer* observer) {
  observers_.insert(observer);
}

void LauncherSearchIconImageLoader::RemoveObserver(Observer* observer) {
  observers_.erase(observer);
}

const gfx::ImageSkia& LauncherSearchIconImageLoader::GetIconImage() const {
  // If no custom icon is supplied, return the extension icon.
  if (custom_icon_image_.isNull())
    return extension_icon_image_;

  return custom_icon_image_;
}

const gfx::ImageSkia& LauncherSearchIconImageLoader::GetBadgeIconImage() const {
  // If a custom icon is supplied, badge it with the extension icon.
  if (!custom_icon_image_.isNull())
    return extension_icon_image_;

  return custom_icon_image_;  // Returns as an empty image.
}

void LauncherSearchIconImageLoader::OnExtensionIconChanged(
    const gfx::ImageSkia& image) {
  CHECK(!image.isNull());

  extension_icon_image_ = image;

  if (custom_icon_image_.isNull()) {
    // When |custom_icon_image_| is not set, extension icon image will be shown
    // as a main icon.
    NotifyObserversIconImageChange();
  } else {
    // When |custom_icon_image_| is set, extension icon image will be shown as a
    // badge icon.
    NotifyObserversBadgeIconImageChange();
  }
}

void LauncherSearchIconImageLoader::OnCustomIconLoaded(
    const gfx::ImageSkia& image) {
  if (image.isNull()) {
    std::vector<std::string> params;
    params.push_back(kWarningMessagePrefix);
    params.push_back(GetTruncatedIconUrl(kTruncatedIconUrlMaxSize));
    error_reporter_->Warn(base::ReplaceStringPlaceholders(
        "$1 Failed to load icon URL: $2", params, nullptr));

    return;
  }

  const bool previously_unbadged = custom_icon_image_.isNull();
  custom_icon_image_ = gfx::ImageSkiaOperations::CreateResizedImage(
      image, skia::ImageOperations::RESIZE_BEST, icon_size_);
  NotifyObserversIconImageChange();

  // If custom_icon_image_ is not set before, extension icon moves from main
  // icon to badge icon. We need to notify badge icon image change after we set
  // custom_icon_image_ to return proper image in GetBadgeIconImage method.
  if (previously_unbadged)
    NotifyObserversBadgeIconImageChange();
}

void LauncherSearchIconImageLoader::NotifyObserversIconImageChange() {
  for (auto* observer : observers_) {
    observer->OnIconImageChanged(this);
  }
}

void LauncherSearchIconImageLoader::NotifyObserversBadgeIconImageChange() {
  for (auto* observer : observers_) {
    observer->OnBadgeIconImageChanged(this);
  }
}

std::string LauncherSearchIconImageLoader::GetTruncatedIconUrl(
    const uint32_t max_size) {
  CHECK(max_size > 3);

  if (icon_url_.spec().size() <= max_size)
    return icon_url_.spec();

  std::string truncated_url;
  base::TruncateUTF8ToByteSize(icon_url_.spec(), max_size - 3, &truncated_url);
  truncated_url.append("...");
  return truncated_url;
}

}  // namespace app_list
