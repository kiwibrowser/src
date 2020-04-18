// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/launcher_search/launcher_search_icon_image_loader_impl.h"

#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "chrome/browser/extensions/extension_util.h"
#include "extensions/browser/image_loader.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_handlers/icons_handler.h"

namespace app_list {

LauncherSearchIconImageLoaderImpl::LauncherSearchIconImageLoaderImpl(
    const GURL& custom_icon_url,
    Profile* profile,
    const extensions::Extension* extension,
    const int icon_dimension,
    std::unique_ptr<chromeos::launcher_search_provider::ErrorReporter>
        error_reporter)
    : LauncherSearchIconImageLoader(custom_icon_url,
                                    profile,
                                    extension,
                                    icon_dimension,
                                    std::move(error_reporter)) {}

LauncherSearchIconImageLoaderImpl::~LauncherSearchIconImageLoaderImpl() =
    default;

const gfx::ImageSkia& LauncherSearchIconImageLoaderImpl::LoadExtensionIcon() {
  extension_icon_image_ = base::WrapUnique(new extensions::IconImage(
      profile_, extension_, extensions::IconsInfo::GetIcons(extension_),
      icon_size_.width(), extensions::util::GetDefaultExtensionIcon(), this));

  return extension_icon_image_->image_skia();
}

void LauncherSearchIconImageLoaderImpl::LoadIconResourceFromExtension() {
  const base::FilePath& file_path =
      extensions::file_util::ExtensionURLToRelativeFilePath(icon_url_);
  const extensions::ExtensionResource& resource =
      extension_->GetResource(file_path);

  // Load image as scale factor 2.0 (crbug.com/490597).
  std::vector<extensions::ImageLoader::ImageRepresentation> info_list;
  info_list.push_back(extensions::ImageLoader::ImageRepresentation(
      resource, extensions::ImageLoader::ImageRepresentation::ALWAYS_RESIZE,
      gfx::Size(icon_size_.width() * 2, icon_size_.height() * 2),
      ui::SCALE_FACTOR_200P));
  extensions::ImageLoader::Get(profile_)->LoadImagesAsync(
      extension_, info_list,
      base::Bind(&LauncherSearchIconImageLoaderImpl::OnCustomIconImageLoaded,
                 this));
}

void LauncherSearchIconImageLoaderImpl::OnExtensionIconImageChanged(
    extensions::IconImage* image) {
  DCHECK_EQ(extension_icon_image_.get(), image);
  OnExtensionIconChanged(extension_icon_image_->image_skia());
}

void LauncherSearchIconImageLoaderImpl::OnCustomIconImageLoaded(
    const gfx::Image& image) {
  OnCustomIconLoaded(image.AsImageSkia());
}

}  // namespace app_list
