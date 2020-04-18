// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_LEVELDB_LEVELDB_SERVICE_IMPL_H_
#define COMPONENTS_SERVICES_LEVELDB_LEVELDB_SERVICE_IMPL_H_

#include "base/memory/ref_counted.h"
#include "components/services/leveldb/leveldb_mojo_proxy.h"
#include "components/services/leveldb/public/interfaces/leveldb.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace base {
class SequencedTaskRunner;
}

namespace leveldb {

// Creates LevelDBDatabases based scoped to a |directory|/|dbname|.
class LevelDBServiceImpl : public mojom::LevelDBService {
 public:
  // The |file_task_runner| is used to run tasks to interact with the
  // file_service. Specifically this task runner must NOT be the same as the
  // task runner this implementation runs on, or deadlock might occur.
  LevelDBServiceImpl(scoped_refptr<base::SequencedTaskRunner> file_task_runner);
  ~LevelDBServiceImpl() override;

  // Overridden from LevelDBService:
  void Open(filesystem::mojom::DirectoryPtr directory,
            const std::string& dbname,
            const base::Optional<base::trace_event::MemoryAllocatorDumpGuid>&
                memory_dump_id,
            leveldb::mojom::LevelDBDatabaseAssociatedRequest database,
            OpenCallback callback) override;
  void OpenWithOptions(
      const leveldb_env::Options& open_options,
      filesystem::mojom::DirectoryPtr directory,
      const std::string& dbname,
      const base::Optional<base::trace_event::MemoryAllocatorDumpGuid>&
          memory_dump_id,
      leveldb::mojom::LevelDBDatabaseAssociatedRequest database,
      OpenCallback callback) override;
  void OpenInMemory(
      const base::Optional<base::trace_event::MemoryAllocatorDumpGuid>&
          memory_dump_id,
      const std::string& tracking_name,
      leveldb::mojom::LevelDBDatabaseAssociatedRequest database,
      OpenInMemoryCallback callback) override;
  void Destroy(filesystem::mojom::DirectoryPtr directory,
               const std::string& dbname,
               DestroyCallback callback) override;

 private:
  // Thread to own the mojo message pipe. Because leveldb spawns multiple
  // threads that want to call file stuff, we create a dedicated thread to send
  // and receive mojo message calls.
  scoped_refptr<LevelDBMojoProxy> thread_;

  DISALLOW_COPY_AND_ASSIGN(LevelDBServiceImpl);
};

}  // namespace leveldb

#endif  // COMPONENTS_SERVICES_LEVELDB_LEVELDB_SERVICE_IMPL_H_
