// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_LEVELDB_LEVELDB_APP_H_
#define COMPONENTS_SERVICES_LEVELDB_LEVELDB_APP_H_

#include <memory>

#include "components/services/leveldb/public/interfaces/leveldb.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

namespace base {
class SequencedTaskRunner;
}

namespace leveldb {

class LevelDBApp : public service_manager::Service {
 public:
  LevelDBApp();
  ~LevelDBApp() override;

 private:
  // |Service| override:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  void Create(leveldb::mojom::LevelDBServiceRequest request);

  std::unique_ptr<mojom::LevelDBService> service_;
  service_manager::BinderRegistry registry_;
  mojo::BindingSet<mojom::LevelDBService> bindings_;

  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(LevelDBApp);
};

}  // namespace leveldb

#endif  // COMPONENTS_SERVICES_LEVELDB_LEVELDB_APP_H_
