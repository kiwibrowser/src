// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/file/file_service.h"

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/task_scheduler/post_task.h"
#include "components/services/filesystem/lock_table.h"
#include "components/services/leveldb/leveldb_service_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/file/file_system.h"
#include "services/file/user_id_map.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace file {

class FileService::FileSystemObjects
    : public base::SupportsWeakPtr<FileSystemObjects> {
 public:
  // Created on the main thread.
  FileSystemObjects(base::FilePath user_dir) : user_dir_(user_dir) {}

  // Destroyed on the |file_service_runner_|.
  ~FileSystemObjects() {}

  // Called on the |file_service_runner_|.
  void OnFileSystemRequest(const service_manager::Identity& remote_identity,
                           mojom::FileSystemRequest request) {
    if (!lock_table_)
      lock_table_ = new filesystem::LockTable;
    mojo::MakeStrongBinding(
        std::make_unique<FileSystem>(user_dir_, lock_table_),
        std::move(request));
  }

 private:
  scoped_refptr<filesystem::LockTable> lock_table_;
  base::FilePath user_dir_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemObjects);
};

class FileService::LevelDBServiceObjects
    : public base::SupportsWeakPtr<LevelDBServiceObjects> {
 public:
  // Created on the main thread.
  LevelDBServiceObjects(
      scoped_refptr<base::SequencedTaskRunner> file_task_runner)
      : file_task_runner_(std::move(file_task_runner)) {}

  // Destroyed on the |leveldb_service_runner_|.
  ~LevelDBServiceObjects() {}

  // Called on the |leveldb_service_runner_|.
  void OnLevelDBServiceRequest(const service_manager::Identity& remote_identity,
                               leveldb::mojom::LevelDBServiceRequest request) {
    if (!leveldb_service_)
      leveldb_service_.reset(
          new leveldb::LevelDBServiceImpl(file_task_runner_));
    leveldb_bindings_.AddBinding(leveldb_service_.get(), std::move(request));
  }

 private:
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  // Variables that are only accessible on the |leveldb_service_runner_| thread.
  std::unique_ptr<leveldb::mojom::LevelDBService> leveldb_service_;
  mojo::BindingSet<leveldb::mojom::LevelDBService> leveldb_bindings_;

  DISALLOW_COPY_AND_ASSIGN(LevelDBServiceObjects);
};

std::unique_ptr<service_manager::Service> CreateFileService() {
  return std::make_unique<FileService>();
}

FileService::FileService()
    : file_service_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      leveldb_service_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskShutdownBehavior::BLOCK_SHUTDOWN})) {
  registry_.AddInterface<leveldb::mojom::LevelDBService>(base::Bind(
      &FileService::BindLevelDBServiceRequest, base::Unretained(this)));
  registry_.AddInterface<mojom::FileSystem>(
      base::Bind(&FileService::BindFileSystemRequest, base::Unretained(this)));
}

FileService::~FileService() {
  file_service_runner_->DeleteSoon(FROM_HERE, file_system_objects_.release());
  leveldb_service_runner_->DeleteSoon(FROM_HERE, leveldb_objects_.release());
}

void FileService::OnStart() {
  file_system_objects_.reset(new FileService::FileSystemObjects(
      GetUserDirForUserId(context()->identity().user_id())));
  leveldb_objects_.reset(
      new FileService::LevelDBServiceObjects(file_service_runner_));
}

void FileService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe),
                          source_info);
}

void FileService::BindFileSystemRequest(
    mojom::FileSystemRequest request,
    const service_manager::BindSourceInfo& source_info) {
  file_service_runner_->PostTask(
      FROM_HERE,
      base::Bind(&FileService::FileSystemObjects::OnFileSystemRequest,
                 file_system_objects_->AsWeakPtr(), source_info.identity,
                 base::Passed(&request)));
}

void FileService::BindLevelDBServiceRequest(
    leveldb::mojom::LevelDBServiceRequest request,
    const service_manager::BindSourceInfo& source_info) {
  leveldb_service_runner_->PostTask(
      FROM_HERE,
      base::Bind(&FileService::LevelDBServiceObjects::OnLevelDBServiceRequest,
                 leveldb_objects_->AsWeakPtr(), source_info.identity,
                 base::Passed(&request)));
}

}  // namespace user_service
