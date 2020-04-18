// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/about_resource_root_folder_id_loader.h"

#include <memory>

#include "components/drive/chromeos/about_resource_loader.h"
#include "google_apis/drive/drive_api_parser.h"

namespace drive {
namespace internal {

AboutResourceRootFolderIdLoader::AboutResourceRootFolderIdLoader(
    AboutResourceLoader* about_resource_loader)
    : about_resource_loader_(about_resource_loader), weak_ptr_factory_(this) {}

AboutResourceRootFolderIdLoader::~AboutResourceRootFolderIdLoader() = default;

void AboutResourceRootFolderIdLoader::GetRootFolderId(
    const RootFolderIdCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // After the initial load GetAboutResource will just return the cached value,
  // avoiding any network calls.
  about_resource_loader_->GetAboutResource(
      base::BindRepeating(&AboutResourceRootFolderIdLoader::OnGetAboutResource,
                          weak_ptr_factory_.GetWeakPtr(), callback));
}

void AboutResourceRootFolderIdLoader::OnGetAboutResource(
    const RootFolderIdCallback& callback,
    google_apis::DriveApiErrorCode status,
    std::unique_ptr<google_apis::AboutResource> about_resource) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  FileError error = GDataToFileError(status);

  if (error != FILE_ERROR_OK) {
    callback.Run(error, base::nullopt);
    return;
  }

  DCHECK(about_resource);

  callback.Run(error, about_resource->root_folder_id());
}

}  // namespace internal
}  // namespace drive
