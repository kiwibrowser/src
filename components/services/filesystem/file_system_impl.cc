// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/filesystem/file_system_impl.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "components/services/filesystem/directory_impl.h"
#include "components/services/filesystem/lock_table.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/identity.h"
#include "url/gurl.h"

namespace filesystem {

FileSystemImpl::FileSystemImpl(const service_manager::Identity& remote_identity,
                               base::FilePath persistent_dir,
                               scoped_refptr<LockTable> lock_table)
    : remote_application_name_(remote_identity.name()),
      lock_table_(std::move(lock_table)),
      persistent_dir_(persistent_dir) {}

FileSystemImpl::~FileSystemImpl() {}

void FileSystemImpl::OpenTempDirectory(mojom::DirectoryRequest directory,
                                       OpenTempDirectoryCallback callback) {
  // Set only if the |DirectoryImpl| will own a temporary directory.
  std::unique_ptr<base::ScopedTempDir> temp_dir(new base::ScopedTempDir);
  CHECK(temp_dir->CreateUniqueTempDir());

  base::FilePath path = temp_dir->GetPath();
  scoped_refptr<SharedTempDir> shared_temp_dir =
      new SharedTempDir(std::move(temp_dir));
  mojo::MakeStrongBinding(std::make_unique<DirectoryImpl>(
                              path, std::move(shared_temp_dir), lock_table_),
                          std::move(directory));
  std::move(callback).Run(base::File::Error::FILE_OK);
}

void FileSystemImpl::OpenPersistentFileSystem(
    mojo::InterfaceRequest<mojom::Directory> directory,
    OpenPersistentFileSystemCallback callback) {
  std::unique_ptr<base::ScopedTempDir> temp_dir;
  base::FilePath path = persistent_dir_;
  if (!base::PathExists(path))
    base::CreateDirectory(path);

  scoped_refptr<SharedTempDir> shared_temp_dir =
      new SharedTempDir(std::move(temp_dir));

  mojo::MakeStrongBinding(std::make_unique<DirectoryImpl>(
                              path, std::move(shared_temp_dir), lock_table_),
                          std::move(directory));
  std::move(callback).Run(base::File::Error::FILE_OK);
}

}  // namespace filesystem
