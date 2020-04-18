// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_FILE_FILE_SERVICE_H_
#define SERVICES_FILE_FILE_SERVICE_H_

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "components/services/filesystem/lock_table.h"
#include "components/services/leveldb/public/interfaces/leveldb.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/file/public/mojom/file_system.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

namespace file {

std::unique_ptr<service_manager::Service> CreateFileService();

class FileService : public service_manager::Service {
 public:
  FileService();
  ~FileService() override;

 private:
  // |Service| override:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  void BindFileSystemRequest(
      mojom::FileSystemRequest request,
      const service_manager::BindSourceInfo& source_info);

  void BindLevelDBServiceRequest(
      leveldb::mojom::LevelDBServiceRequest request,
      const service_manager::BindSourceInfo& source_info);

  void OnLevelDBServiceError();

  scoped_refptr<base::SequencedTaskRunner> file_service_runner_;
  scoped_refptr<base::SequencedTaskRunner> leveldb_service_runner_;

  // We create these two objects so we can delete them on the correct task
  // runners.
  class FileSystemObjects;
  std::unique_ptr<FileSystemObjects> file_system_objects_;

  class LevelDBServiceObjects;
  std::unique_ptr<LevelDBServiceObjects> leveldb_objects_;

  service_manager::BinderRegistryWithArgs<
      const service_manager::BindSourceInfo&>
      registry_;

  DISALLOW_COPY_AND_ASSIGN(FileService);
};

}  // namespace file

#endif  // SERVICES_FILE_FILE_SERVICE_H_
