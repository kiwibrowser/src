// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_FILESYSTEM_FILE_SYSTEM_APP_H_
#define COMPONENTS_SERVICES_FILESYSTEM_FILE_SYSTEM_APP_H_

#include "base/macros.h"
#include "components/services/filesystem/directory_impl.h"
#include "components/services/filesystem/file_system_impl.h"
#include "components/services/filesystem/lock_table.h"
#include "components/services/filesystem/public/interfaces/file_system.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

namespace filesystem {

class FileSystemApp : public service_manager::Service {
 public:
  FileSystemApp();
  ~FileSystemApp() override;

 private:
  // Gets the system specific toplevel profile directory.
  static base::FilePath GetUserDataDir();

  // |service_manager::Service| override:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  void Create(mojom::FileSystemRequest request,
              const service_manager::BindSourceInfo& source_info);

  service_manager::BinderRegistryWithArgs<
      const service_manager::BindSourceInfo&>
      registry_;

  scoped_refptr<LockTable> lock_table_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemApp);
};

}  // namespace filesystem

#endif  // COMPONENTS_SERVICES_FILESYSTEM_FILE_SYSTEM_APP_H_
