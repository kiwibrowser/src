// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_FILE_UTIL_ZIP_FILE_CREATOR_H_
#define CHROME_SERVICES_FILE_UTIL_ZIP_FILE_CREATOR_H_

#include <vector>

#include "chrome/services/file_util/public/mojom/zip_file_creator.mojom.h"
#include "components/services/filesystem/public/interfaces/directory.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace base {
class FilePath;
}

namespace chrome {

class ZipFileCreator : public chrome::mojom::ZipFileCreator {
 public:
  explicit ZipFileCreator(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~ZipFileCreator() override;

 private:
  // chrome::mojom::ZipFileCreator:
  void CreateZipFile(filesystem::mojom::DirectoryPtr source_dir_mojo,
                     const base::FilePath& source_dir,
                     const std::vector<base::FilePath>& source_relative_paths,
                     base::File zip_file,
                     CreateZipFileCallback callback) override;

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(ZipFileCreator);
};

}  // namespace chrome

#endif  // CHROME_SERVICES_FILE_UTIL_ZIP_FILE_CREATOR_H_
