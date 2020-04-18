// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/file/file_system.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/utf_string_conversions.h"
#include "components/services/filesystem/directory_impl.h"
#include "components/services/filesystem/lock_table.h"
#include "components/services/filesystem/public/interfaces/types.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace file {

FileSystem::FileSystem(const base::FilePath& base_user_dir,
                       const scoped_refptr<filesystem::LockTable>& lock_table)
    : lock_table_(lock_table), path_(base_user_dir) {
  base::CreateDirectory(path_);
}

FileSystem::~FileSystem() {}

void FileSystem::GetDirectory(filesystem::mojom::DirectoryRequest request,
                              GetDirectoryCallback callback) {
  mojo::MakeStrongBinding(
      std::make_unique<filesystem::DirectoryImpl>(
          path_, scoped_refptr<filesystem::SharedTempDir>(), lock_table_),
      std::move(request));
  std::move(callback).Run();
}

void FileSystem::GetSubDirectory(const std::string& sub_directory_path,
                                 filesystem::mojom::DirectoryRequest request,
                                 GetSubDirectoryCallback callback) {
  // Ensure that we've made |subdirectory| recursively under our user dir.
  base::FilePath subdir = path_.Append(
#if defined(OS_WIN)
      base::UTF8ToWide(sub_directory_path));
#else
      sub_directory_path);
#endif
  base::File::Error error;
  if (!base::CreateDirectoryAndGetError(subdir, &error)) {
    std::move(callback).Run(error);
    return;
  }

  mojo::MakeStrongBinding(
      std::make_unique<filesystem::DirectoryImpl>(
          subdir, scoped_refptr<filesystem::SharedTempDir>(), lock_table_),
      std::move(request));
  std::move(callback).Run(base::File::Error::FILE_OK);
}

}  // namespace file
