// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_ABOUT_RESOURCE_ROOT_FOLDER_ID_LOADER_H_
#define COMPONENTS_DRIVE_CHROMEOS_ABOUT_RESOURCE_ROOT_FOLDER_ID_LOADER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "components/drive/chromeos/root_folder_id_loader.h"
#include "google_apis/drive/drive_api_error_codes.h"

namespace google_apis {
class AboutResource;
}  // namespace google_apis

namespace drive {
namespace internal {

class AboutResourceLoader;

// Retrieves the root folder id from the about resource loader. This is used
// to get the root folder ID for the users default corpus. As the value is
// constant we just use GetAboutResource which will usually retrieve a
// cached value.
class AboutResourceRootFolderIdLoader : public RootFolderIdLoader {
 public:
  explicit AboutResourceRootFolderIdLoader(
      AboutResourceLoader* about_resource_loader);
  ~AboutResourceRootFolderIdLoader() override;

  void GetRootFolderId(const RootFolderIdCallback& callback) override;

 private:
  void OnGetAboutResource(
      const RootFolderIdCallback& callback,
      google_apis::DriveApiErrorCode error,
      std::unique_ptr<google_apis::AboutResource> about_resource);

  AboutResourceLoader* about_resource_loader_;  // Not owned.

  THREAD_CHECKER(thread_checker_);

  base::WeakPtrFactory<AboutResourceRootFolderIdLoader> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(AboutResourceRootFolderIdLoader);
};

}  // namespace internal
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_ABOUT_RESOURCE_ROOT_FOLDER_ID_LOADER_H_
